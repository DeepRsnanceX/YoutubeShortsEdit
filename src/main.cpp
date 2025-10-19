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

class $modify(ShortsEditPL, PlayLayer) {
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		pausedByMod = false;
		gonnaPause = false;

		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto spr = CCSprite::createWithSpriteFrameName("editImg_1.png"_spr);
		spr->setPosition({winSize.width / 2.f, 40.f});
		spr->setVisible(false);
		spr->setID("no-description-needed"_spr);
		this->addChild(spr, 1000);

		auto vignetteSpr = CCSprite::createWithSpriteFrameName("vignette.png"_spr);
		vignetteSpr->setPosition({winSize.width / 2.f, winSize.height / 2.f});
		vignetteSpr->setScaleX(winSize.width / vignetteSpr->getContentSize().width);
		vignetteSpr->setScaleY(winSize.height / vignetteSpr->getContentSize().height);
		vignetteSpr->setID("edit-vignette"_spr);
		vignetteSpr->setVisible(false);
		this->addChild(vignetteSpr, 1001);
	}
};

class $modify(ShortsEditPO, PlayerObject) {
	void thoseWhoKnow(float dt) {
		auto playLayer = PlayLayer::get();
		if (!playLayer) return;

		auto yeah = static_cast<CCSprite*>(playLayer->getChildByID("no-description-needed"_spr));
		if (!yeah) return;

		auto vign = playLayer->getChildByID("edit-vignette"_spr);
		if (!vign) return;

		vign->setVisible(true);
		yeah->setVisible(true);
		std::string frameName = fmt::format("editImg_{}.png"_spr, getRandInt(1, 6));
		auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
		if (frame) yeah->setDisplayFrame(frame);

		pausedByMod = true;
		playLayer->m_uiLayer->m_pauseBtn->activate();
	}

	bool pushButton(PlayerButton p0) {
		if (!PlayerObject::pushButton(p0)) return false;

		auto playLayer = PlayLayer::get();
		if (!playLayer) return true;
		
		if (pausedByMod) return true;
		if (gonnaPause) return true;
		
		int chance = getRandInt(0, 100);
		if (chance >= 50) {
			gonnaPause = true;
			this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), 0.1f);
		}

		return true;
	}
};

class $modify(ShortsEditPauseLayer, PauseLayer) {
	void okayDude(float dt) {
		PauseLayer::onResume(nullptr);
		pausedByMod = false;
		gonnaPause = false;

		auto pl = PlayLayer::get();
		if (!pl) return;

		auto lmao = pl->getChildByID("no-description-needed"_spr);
		if (!lmao) return;

		auto vign = pl->getChildByID("edit-vignette"_spr);
		if (!vign) return;

		lmao->setVisible(false);
		vign->setVisible(false);
	}

	void customSetup() {
		PauseLayer::customSetup();

		auto playLayer = PlayLayer::get();
		if (!playLayer) return;

		this->setVisible(!pausedByMod);
		
		if (pausedByMod) {
			std::string phonkSound = fmt::format("phonk_{}.ogg"_spr, getRandInt(1, 4));
			
			auto fmod = FMODAudioEngine::sharedEngine();

			fmod->resumeAllEffects();
			fmod->playEffect(phonkSound.c_str());

			this->scheduleOnce(schedule_selector(ShortsEditPauseLayer::okayDude), 1.5f);
		}
	}
};