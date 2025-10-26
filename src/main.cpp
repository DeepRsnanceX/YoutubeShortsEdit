#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <random>

#ifndef GEODE_IS_IOS
#include <geode.custom-keybinds/include/Keybinds.hpp>
using namespace keybinds;
#endif

using namespace geode::prelude;

int getRandInt(int min, int max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(min, max);
	return distrib(gen);
}

struct ButtonPositionData {
	float xPos;
	float yPos;
	float xAnchor;
	float yAnchor;
};

$on_mod(Loaded) {
	#ifndef GEODE_IS_IOS
	BindManager::get()->registerBindable({
		"activate-phonk-edit"_spr,
		"Trigger Phonk Edit Manually",
		"Manually triggers the phonk edit effect.",
		{ Keybind::create(KEY_V, Modifier::None) },
		Mod::get()->getName()
	});
	#endif
}

bool pausedByMod = false;
bool gonnaPause = false;
bool canPlayEffect = false;

class $modify(ShortsEditPL, PlayLayer) {
	struct Fields {
		Ref<CCRenderTexture> plRenderer = nullptr;
		CCSpriteGrayscale* grayscreen = nullptr;

		std::vector<std::string> builtinImages;
		std::vector<std::string> customImages;
		std::vector<std::string> builtinSounds;
		std::vector<std::string> customSounds;
	};

	void updateReleaseValidPL(float dt) {
		canPlayEffect = true;
		this->unschedule(schedule_selector(ShortsEditPL::updateReleaseValidPL));
	}

	void loadCustomAssets() {
		auto fields = m_fields.self();
		
		fields->customImages.clear();
		fields->customSounds.clear();
		
		fields->builtinImages.clear();
		for (int i = 1; i <= 14; i++) {
			fields->builtinImages.push_back(fmt::format("editImg_{}.png"_spr, i));
		}
		
		fields->builtinSounds.clear();
		for (int i = 1; i <= 26; i++) {
			fields->builtinSounds.push_back(fmt::format("phonk_{}.ogg"_spr, i));
		}
		
		if (Mod::get()->getSettingValue<bool>("include-custom-images")) {
			auto imagesDir = Mod::get()->getConfigDir(true) / "images";
			
			if (std::filesystem::exists(imagesDir) && std::filesystem::is_directory(imagesDir)) {
				for (const auto& entry : std::filesystem::directory_iterator(imagesDir)) {
					if (entry.is_regular_file()) {
						//auto ext = entry.path().extension().string();
						auto ext = utils::string::pathToString(entry.path().extension());
						if (ext == ".png") {
							fields->customImages.push_back(utils::string::pathToString(entry.path()));
						}
					}
				}
			}
		}
		
		if (Mod::get()->getSettingValue<bool>("include-custom-sounds")) {
			auto soundsDir = Mod::get()->getConfigDir(true) / "phonk";
			
			if (std::filesystem::exists(soundsDir) && std::filesystem::is_directory(soundsDir)) {
				for (const auto& entry : std::filesystem::directory_iterator(soundsDir)) {
					if (entry.is_regular_file()) {
						//auto ext = entry.path().extension().string();
						auto ext = utils::string::pathToString(entry.path().extension());
						if (ext == ".ogg" || ext == ".mp3" || ext == ".wav") {
							fields->customSounds.push_back(utils::string::pathToString(entry.path()));
						}
					}
				}
			}
		}
	}

	std::string getRandomImage() {
		auto fields = m_fields.self();
		std::vector<std::string> allImages;
		
		if (Mod::get()->getSettingValue<bool>("include-builtin-images")) {
			allImages.insert(allImages.end(), fields->builtinImages.begin(), fields->builtinImages.end());
		}
		
		if (Mod::get()->getSettingValue<bool>("include-custom-images")) {
			allImages.insert(allImages.end(), fields->customImages.begin(), fields->customImages.end());
		}
		
		if (allImages.empty()) return fields->builtinImages[0];
		
		return allImages[getRandInt(0, allImages.size() - 1)];
	}

	std::string getRandomSound() {
		auto fields = m_fields.self();
		std::vector<std::string> allSounds;
		
		if (Mod::get()->getSettingValue<bool>("include-builtin-sounds")) {
			allSounds.insert(allSounds.end(), fields->builtinSounds.begin(), fields->builtinSounds.end());
		}
		
		if (Mod::get()->getSettingValue<bool>("include-custom-sounds")) {
			allSounds.insert(allSounds.end(), fields->customSounds.begin(), fields->customSounds.end());
		}
		
		if (allSounds.empty()) return fields->builtinSounds[0];
		
		return allSounds[getRandInt(0, allSounds.size() - 1)];
	}

	// credit to zilko & ery's grayscale mode mod for the CCEGLView and Director shenanigans
	// i would've NEVER figured ANY of this out on my own
	// (fixes the grayscale "screenshot" when shaders are active)
	CCTexture2D* renderPL() {
		auto fields = m_fields.self();
		if (!fields->plRenderer) return nullptr;

		if (fields->grayscreen) fields->grayscreen->setVisible(false);

		auto view = CCEGLView::get();
		auto director = CCDirector::sharedDirector();
		
		auto winSize = director->getWinSize();
		auto ogRes = view->getDesignResolutionSize();

		float ogX = view->m_fScaleX;
		float ogY = view->m_fScaleY;

		CCSize size = {roundf(320.f * (winSize.width / winSize.height)), 320.f};
		CCSize newScale = {winSize.width / size.width, winSize.height / size.height};
		float scale = director->getContentScaleFactor() / utils::getDisplayFactor();
		
		director->m_obWinSizeInPoints = size;
		view->setDesignResolutionSize(size.width, size.height, ResolutionPolicy::kResolutionExactFit);
		view->m_fScaleX = scale * newScale.width;
		view->m_fScaleY = scale * newScale.height;

		fields->plRenderer->beginWithClear(0, 0, 0, 0);
		this->visit();
		fields->plRenderer->end();

		director->m_obWinSizeInPoints = ogRes;
		view->setDesignResolutionSize(ogRes.width, ogRes.height, ResolutionPolicy::kResolutionExactFit);
		view->m_fScaleX = ogX;
		view->m_fScaleY = ogY;

		return fields->plRenderer->getSprite()->getTexture();
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		auto fields = m_fields.self();
		canPlayEffect = false;
		this->scheduleOnce(schedule_selector(ShortsEditPL::updateReleaseValidPL), Mod::get()->getSettingValue<double>("action-cooldown"));

		if (gonnaPause) gonnaPause = false;
		pausedByMod = false;

		if (!fields->grayscreen) return;
		if (fields->grayscreen->isVisible()) fields->grayscreen->setVisible(false);

		auto vign = this->getChildByID("edit-vignette"_spr);
		if (!vign) return;
		if (vign->isVisible()) vign->setVisible(false);

		auto editImg = this->getChildByID("no-description-needed"_spr);
		if (!editImg) return;
		if (editImg->isVisible()) editImg->setVisible(false);
	}

	void levelComplete() {
		PlayLayer::levelComplete();
		auto fields = m_fields.self();
		canPlayEffect = false;

		if (!fields->grayscreen) return;
		if (fields->grayscreen->isVisible()) fields->grayscreen->setVisible(false);

		if (Mod::get()->getSettingValue<bool>("hide-on-complete")) {
			auto vignette = this->getChildByID("edit-vignette"_spr);
			if (vignette) vignette->setVisible(false);

			auto editImg = this->getChildByID("no-description-needed"_spr);
			if (editImg) editImg->setVisible(false);
		}
		
	}

	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		pausedByMod = false;
		gonnaPause = false;
		canPlayEffect = true;
		auto fields = m_fields.self();
		auto winSize = CCDirector::sharedDirector()->getWinSize();

		float sprY = winSize.height / 2.f;
		float sprYOffset = Mod::get()->getSettingValue<double>("image-yoffset");

		ShortsEditPL::loadCustomAssets();

		fields->plRenderer = CCRenderTexture::create(winSize.width, winSize.height);

		fields->grayscreen = CCSpriteGrayscale::createWithTexture(renderPL());
		fields->grayscreen->setAnchorPoint({0.f, 0.f});
		fields->grayscreen->setPosition({0.f, 0.f});
		fields->grayscreen->setFlipY(true);
		fields->grayscreen->setID("grayscale-screenshot"_spr);
		fields->grayscreen->setVisible(false);
		m_uiLayer->addChild(fields->grayscreen, 10000);

		auto spr = CCSprite::createWithSpriteFrameName("editImg_1.png"_spr);
		spr->setPosition({winSize.width / 2.f, sprY + sprYOffset});
		spr->setScale(Mod::get()->getSettingValue<double>("image-scale"));
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

		if (!canPlayEffect || pausedByMod) return;

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

		std::string chosenImg = static_cast<ShortsEditPL*>(playLayer)->getRandomImage();
		
		if (chosenImg.find("editImg_") != std::string::npos) {
			auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(chosenImg.c_str());
			if (frame) {
				yeah->setDisplayFrame(frame);
			}
		} else {
			auto texture = CCTextureCache::sharedTextureCache()->addImage(chosenImg.c_str(), false);
			if (texture) {
				yeah->setTexture(texture);
				yeah->setTextureRect(CCRect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
			}
		}

		gonnaPause = false;
		pausedByMod = true;
		playLayer->m_uiLayer->m_pauseBtn->activate();
	}

	void updateReleaseValid(float dt) {
        canPlayEffect = true;
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

		if (Mod::get()->getSettingValue<bool>("trigger-manually")) return true;

		auto playLayer = PlayLayer::get();
		if (!playLayer) return true;
		auto baseLayer = GJBaseGameLayer::get();
		if (!baseLayer) return true;

		if (pausedByMod || gonnaPause) return true;
		if (!canPlayEffect) return true;
		if (Mod::get()->getSettingValue<std::string>("mod-mode") != "On Click") return true;
		if (!isButtonEnabled(p0)) return true;

		// plat fix
		if (baseLayer->m_isPlatformer) {
			if (!Mod::get()->getSettingValue<bool>("enable-in-plat")) return true;
			
			int chance = getRandInt(0, 100);
			if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), 
					Mod::get()->getSettingValue<double>("action-delay"));
			}
			return true;
		}
		
		// superior mode (normal)
		int percent = playLayer->getCurrentPercentInt();
		int chance = getRandInt(0, 100);
		if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
			if (percent >= Mod::get()->getSettingValue<int64_t>("only-after") && percent <= Mod::get()->getSettingValue<int64_t>("only-before")) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
			}
		}

		return true;
	}

	bool releaseButton(PlayerButton p0) {
		if (!PlayerObject::releaseButton(p0)) return false;
		
		if (Mod::get()->getSettingValue<bool>("trigger-manually")) return true;

		auto playLayer = PlayLayer::get();
		if (!playLayer) return true;
		auto baseLayer = GJBaseGameLayer::get();
		if (!baseLayer) return true;

		if (pausedByMod || gonnaPause) return true;
		if (!canPlayEffect) return true;
		if (!baseLayer->m_isPlatformer && playLayer->getCurrentPercentInt() == 100) return true;
		if (Mod::get()->getSettingValue<std::string>("mod-mode") != "On Release") return true;
		if (!isButtonEnabled(p0)) return true;

		// plat fix
		if (baseLayer->m_isPlatformer) {
			if (!Mod::get()->getSettingValue<bool>("enable-in-plat")) return true;
			
			int chance = getRandInt(0, 100);
			if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
			}
			return true;
		}

		// superior mode (normal)
		int percent = playLayer->getCurrentPercentInt();
		int thresholdMin = (Mod::get()->getSettingValue<int64_t>("only-after") == 0) ? 1 : Mod::get()->getSettingValue<int64_t>("only-after");
		
		int chance = getRandInt(0, 100);
		if (chance >= Mod::get()->getSettingValue<int64_t>("edit-rarity")) {
			if (percent >= thresholdMin && 
				percent <= Mod::get()->getSettingValue<int64_t>("only-before")) {
				gonnaPause = true;
				this->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
			}
		}

		return true;
	}
};

class $modify(ShortsEditGJBGL, GJBaseGameLayer) {
	ButtonPositionData getButtonPosData(std::string chosenSetting) {
		ButtonPositionData data;
		float safeZone = 7.f;

		log::info("Chosen Button Position: {}", chosenSetting);

		if (chosenSetting == "Top Left") {
			data.xPos = safeZone;
			data.yPos = CCDirector::sharedDirector()->getWinSize().height - safeZone;
			data.xAnchor = 0.f;
			data.yAnchor = 1.f;
		} else if (chosenSetting == "Bottom Left") {
			data.xPos = safeZone;
			data.yPos = safeZone;
			data.xAnchor = 0.f;
			data.yAnchor = 0.f;
		} else if (chosenSetting == "Bottom Right") {
			data.xPos = CCDirector::sharedDirector()->getWinSize().width - safeZone;
			data.yPos = safeZone;
			data.xAnchor = 1.f;
			data.yAnchor = 0.f;
		} else {
			// default to bottom right cuz that's the better pos
			data.xPos = CCDirector::sharedDirector()->getWinSize().width - safeZone;
			data.yPos = safeZone;
			data.xAnchor = 1.f;
			data.yAnchor = 0.f;
		}

		return data;
	}

	void activateManually() {
		auto playLayer = PlayLayer::get();
		if (!playLayer) return;
		
		if (!Mod::get()->getSettingValue<bool>("trigger-manually")) return;
		if (pausedByMod || gonnaPause || !canPlayEffect) return;
		
		if (!m_isPlatformer && playLayer->getCurrentPercentInt() == 100) return;
		
		auto player = this->m_player1;
		if (!player) return;
		
		gonnaPause = true;
		static_cast<ShortsEditPO*>(player)->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
	}

	void literallyTheSameThingButForTheButton(CCObject* sender) {
		auto playLayer = PlayLayer::get();
		if (!playLayer) return;
		
		if (!Mod::get()->getSettingValue<bool>("trigger-manually")) return;
		if (pausedByMod || gonnaPause || !canPlayEffect) return;
		
		if (!m_isPlatformer && playLayer->getCurrentPercentInt() == 100) return;
		
		auto player = this->m_player1;
		if (!player) return;
		
		gonnaPause = true;
		static_cast<ShortsEditPO*>(player)->scheduleOnce(schedule_selector(ShortsEditPO::thoseWhoKnow), Mod::get()->getSettingValue<double>("action-delay"));
	}

	bool init() {
		if (!GJBaseGameLayer::init()) return false;
		
		if (!Mod::get()->getSettingValue<bool>("trigger-manually")) return true;

		#ifndef GEODE_IS_IOS
		this->addEventListener<InvokeBindFilter>([=, this](InvokeBindEvent* event) {
			if (event->isDown()) {
				activateManually();
			}
			return ListenerResult::Propagate;
		}, "activate-phonk-edit"_spr);
		#endif
		
		if (Mod::get()->getSettingValue<bool>("enable-button")) {
			// jajaj dise posdata
			ButtonPositionData btnPosData = getButtonPosData(Mod::get()->getSettingValue<std::string>("button-pos"));

			auto btnSpr = CCSprite::createWithSpriteFrameName("manualBtn.png"_spr);
			btnSpr->setOpacity(75);

			auto btn = CCMenuItemSpriteExtra::create(
				btnSpr, 
				this, 
				menu_selector(ShortsEditGJBGL::literallyTheSameThingButForTheButton)
			);

			auto menu = CCMenu::create();
			menu->setContentSize(btnSpr->getContentSize());
			menu->setPosition({btnPosData.xPos, btnPosData.yPos});
			menu->setAnchorPoint({btnPosData.xAnchor, btnPosData.yAnchor});
			menu->ignoreAnchorPointForPosition(false);
			menu->setID("manual-phonk-btn"_spr);
			menu->addChild(btn);

			btn->setPosition({menu->getContentSize().width / 2.f, menu->getContentSize().height / 2.f});

			m_uiLayer->addChild(menu);
		}

		return true;
	}
};

class $modify(ShortsEditPauseLayer, PauseLayer) {
	void okayDude(float dt) {
		pausedByMod = false;

		FMODAudioEngine::sharedEngine()->stopAllEffects();

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

		auto plFields = static_cast<ShortsEditPL*>(playLayer)->m_fields.self();
		if (!plFields) return;

		if (gonnaPause) gonnaPause = false;
		
		if (pausedByMod) {
			this->setPositionY(3000.f);
			std::string phonkSound = static_cast<ShortsEditPL*>(playLayer)->getRandomSound();
			
			auto fmod = FMODAudioEngine::sharedEngine();
			fmod->resumeAllEffects();
			fmod->playEffect(phonkSound.c_str());

			this->scheduleOnce(schedule_selector(ShortsEditPauseLayer::okayDude), Mod::get()->getSettingValue<double>("effect-duration"));
		}
	}

	void onResume(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onResume(sender);

		auto pl = PlayLayer::get();
		if (!pl) return;
		auto player = pl->m_player1;
		if (!player) return;
		auto plFields = static_cast<ShortsEditPL*>(pl)->m_fields.self();
		if (!plFields) return;

		canPlayEffect = false;
		static_cast<ShortsEditPO*>(player)->scheduleOnce(schedule_selector(ShortsEditPO::updateReleaseValid), Mod::get()->getSettingValue<double>("action-cooldown"));
		
		if (gonnaPause) gonnaPause = false;
	}
	void onQuit(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onQuit(sender);
	}
	void tryQuit(CCObject* sender) {
		if (!pausedByMod) PauseLayer::tryQuit(sender);
	}
	void onRestart(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onRestart(sender);
	}
	void onRestartFull(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onRestartFull(sender);
	}
	void onPracticeMode(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onPracticeMode(sender);
	}
	void onNormalMode(CCObject* sender) {
		if (!pausedByMod) PauseLayer::onNormalMode(sender);
	}
};