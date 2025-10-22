#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <random>

using namespace geode::prelude;

int getRandInt(int min, int max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(min, max);
	return distrib(gen);
}

bool pausedByMod = false;
bool gonnaPause = false;
bool isReleaseValid = false;

class $modify(ShortsEditPL, PlayLayer) {
	struct Fields {
		Ref<CCRenderTexture> plRenderer = nullptr;
		CCSpriteGrayscale* grayscreen = nullptr;
	};

	void updateReleaseValidPL(float dt) {
		isReleaseValid = true;
		this->unschedule(schedule_selector(ShortsEditPL::updateReleaseValidPL));
	}

	CCTexture2D* renderPL() {
		auto fields = m_fields.self();
		if (!fields->plRenderer) return nullptr;

		if (fields->grayscreen) fields->grayscreen->setVisible(false);

		fields->plRenderer->beginWithClear(0, 0, 0, 0);
		this->visit();
		fields->plRenderer->end();

		return fields->plRenderer->getSprite()->getTexture();
	}

	void postUpdate(float p0) {
		PlayLayer::postUpdate(p0);

		log::info("{}", isReleaseValid);
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		isReleaseValid = false;
		this->scheduleOnce(schedule_selector(ShortsEditPL::updateReleaseValidPL), 0.5f);
	}

	void levelComplete() {
		PlayLayer::levelComplete();
		isReleaseValid = false;
	}

	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		pausedByMod = false;
		gonnaPause = false;
		isReleaseValid = true;
		auto fields = m_fields.self();
		auto winSize = CCDirector::sharedDirector()->getWinSize();

		fields->plRenderer = CCRenderTexture::create(winSize.width, winSize.height);

		fields->grayscreen = CCSpriteGrayscale::createWithTexture(renderPL());
		fields->grayscreen->setAnchorPoint({0.f, 0.f});
		fields->grayscreen->setPosition({0.f, 0.f});
		fields->grayscreen->setFlipY(true);
		fields->grayscreen->setID("grayscale-screenshot"_spr);
		fields->grayscreen->setVisible(false);
		this->addChild(fields->grayscreen, 10000);

		auto spr = CCSprite::createWithSpriteFrameName("editImg_1.png"_spr);
		spr->setPosition({winSize.width / 2.f, 40.f});
		spr->setVisible(false);
		spr->setID("no-description-needed"_spr);
		this->addChild(spr, 10001);

		auto vignetteSpr = CCSprite::createWithSpriteFrameName("vignette.png"_spr);
		vignetteSpr->setPosition({winSize.width / 2.f, winSize.height / 2.f});
		vignetteSpr->setScaleX(winSize.width / vignetteSpr->getContentSize().width);
		vignetteSpr->setScaleY(winSize.height / vignetteSpr->getContentSize().height);
		vignetteSpr->setID("edit-vignette"_spr);
		vignetteSpr->setVisible(false);
		this->addChild(vignetteSpr, 10002);
	}
};

class $modify(ShortsEditPO, PlayerObject) {
	void thoseWhoKnow(float dt) {
		auto playLayer = PlayLayer::get();
		if (!playLayer) return;

		auto plFields = static_cast<ShortsEditPL*>(playLayer)->m_fields.self();
		if (!plFields) return;

		auto yeah = static_cast<CCSprite*>(playLayer->getChildByID("no-description-needed"_spr));
		if (!yeah) return;

		auto vign = playLayer->getChildByID("edit-vignette"_spr);
		if (!vign) return;

		if (!plFields->grayscreen) return;

		if (plFields->grayscreen && plFields->plRenderer) {
			auto rendered = static_cast<ShortsEditPL*>(playLayer)->renderPL();
			if (rendered) {
				plFields->grayscreen->setTexture(rendered);
				plFields->grayscreen->setTextureRect(CCRect(0, 0, rendered->getContentSize().width, rendered->getContentSize().height));
			}
		}
		
		vign->setVisible(true);
		yeah->setVisible(true);
		plFields->grayscreen->setVisible(true);

		std::string frameName = fmt::format("editImg_{}.png"_spr, getRandInt(1, 14));
		auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
		if (frame) yeah->setDisplayFrame(frame);

		pausedByMod = true;
		playLayer->m_uiLayer->m_pauseBtn->activate();
	}

	void updateReleaseValid(float dt) {
        isReleaseValid = true;
        this->unschedule(schedule_selector(ShortsEditPO::updateReleaseValid));
    }

	bool isButtonEnabled(PlayerButton btn) {
		switch(static_cast<int>(btn)) {
			case 1: return Mod::get()->getSettingValue<bool>("allow-jumpbtn");
			case 2: return Mod::get()->getSettingValue<bool>("allow-leftbtn");
			case 3: return Mod::get()->getSettingValue<bool>("allow-rightbtn");
			default: return false;
		}
	}

	bool pushButton(PlayerButton p0) {
		if (!PlayerObject::pushButton(p0)) return false;

		auto playLayer = PlayLayer::get();
		if (!playLayer) return true;
		if (pausedByMod) return true;
		if (gonnaPause) return true;

		if (Mod::get()->getSettingValue<std::string>("mod-mode") != "On Click") return true;

		if (!isButtonEnabled(p0)) return true;
		
		int percent = playLayer->getCurrentPercentInt();
		int chance = getRandInt(0, 100);
		if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
			if (percent >= Mod::get()->getSettingValue<int64_t>("only-after")) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
			}
		}

		return true;
	}

	bool releaseButton(PlayerButton p0) {
		if (!PlayerObject::releaseButton(p0)) return false;
		
		auto playLayer = PlayLayer::get();
		if (!playLayer) return true;
		if (pausedByMod || gonnaPause) return true;
		if (!isReleaseValid) return true;
		if (playLayer->getCurrentPercentInt() == 100) return true;

		if (Mod::get()->getSettingValue<std::string>("mod-mode") != "On Release") return true;

		if (!isButtonEnabled(p0)) return true;

		int percent = playLayer->getCurrentPercentInt();
		int threshold = (Mod::get()->getSettingValue<int64_t>("only-after") == 0) ? 1 : Mod::get()->getSettingValue<int64_t>("only-after");
		log::info("threshold is: {}", threshold);
		
		int chance = getRandInt(0, 100);

		if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
			if (percent >= threshold) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
			}
		}

		return true;
	}
};

class $modify(ShortsEditPauseLayer, PauseLayer) {
	void okayDude(float dt) {
		pausedByMod = false;
		gonnaPause = false;
		isReleaseValid = false;

		PauseLayer::onResume(nullptr);

		auto pl = PlayLayer::get();
		if (!pl) return;

		auto plFields = static_cast<ShortsEditPL*>(pl)->m_fields.self();
		if (!plFields) return;

		auto lmao = pl->getChildByID("no-description-needed"_spr);
		auto vign = pl->getChildByID("edit-vignette"_spr);

		if (lmao) lmao->setVisible(false);
		if (vign) vign->setVisible(false);
		if (plFields->grayscreen) plFields->grayscreen->setVisible(false);
	}

	void customSetup() {
		PauseLayer::customSetup();

		auto playLayer = PlayLayer::get();
		if (!playLayer) return;
		
		if (pausedByMod) {
			this->setPositionY(3000.f);
			std::string phonkSound = fmt::format("phonk_{}.ogg"_spr, getRandInt(1, 21));
			
			auto fmod = FMODAudioEngine::sharedEngine();

			fmod->resumeAllEffects();
			fmod->playEffect(phonkSound.c_str());

			this->scheduleOnce(schedule_selector(ShortsEditPauseLayer::okayDude), 1.5f);
		}
	}

	void onResume(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onResume(sender);

		auto pl = PlayLayer::get();
		if (!pl) return;

		auto player = pl->m_player1;
        if (player) static_cast<ShortsEditPO*>(player)->scheduleOnce(schedule_selector(ShortsEditPO::updateReleaseValid), 0.5f);
	}
	void onQuit(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onQuit(sender);
	}
	void tryQuit(CCObject* sender) {
		if (!pausedByMod) PauseLayer::tryQuit(sender);
	}
};