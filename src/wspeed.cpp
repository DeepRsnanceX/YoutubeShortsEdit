#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <random>

using namespace geode::prelude;

float randomFloat(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return static_cast<float>(distrib(gen));
}

float realRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distrib(min, max);
    return distrib(gen);
}

struct WSpeedFontData {
    ccColor3B color;
    std::string text;
};

class $modify(WSpeedBaseLayer, GJBaseGameLayer) {
    struct Fields {
        CCLabelBMFont* actionFont = nullptr;
    };

    WSpeedFontData getFontData(GJGameEvent event) {
        WSpeedFontData data;
        int eventAsInt = static_cast<int>(event);
        ccColor3B playerColor = GameManager::sharedState()->colorForIdx(GameManager::sharedState()->getPlayerColor());

        switch(eventAsInt) {
            case 23:
                data.color = ccColor3B({0, 255, 0});
                data.text = "*DASHES*";
                break;
            case 24:
                data.color = ccColor3B({255, 0, 0});
                data.text = "*STOPS DASH*";
                break;
            case 25:
                data.color = playerColor;
                data.text = "*TELEPORTS*";
                break;
            case 26:
                data.color = ccColor3B({0, 255, 0});
                data.text = "*TURNS INTO CUBE*";
                break;
            case 27:
                data.color = ccColor3B({ 255, 0, 170 });
                data.text = "*TURNS INTO SHIP*";
                break;
            case 28:
                data.color = ccColor3B({ 255, 30, 0 });
                data.text = "*TURNS INTO BALL*";
                break;
            case 29:
                data.color = ccColor3B({ 255, 166, 0 });
                data.text = "*TURNS INTO UFO*";
                break;
            case 30:
                data.color = ccColor3B({ 0, 238, 255 });
                data.text = "*TURNS INTO WAVE*";
                break;
            case 31:
                data.color = ccColor3B({255, 255, 255});
                data.text = "*TURNS INTO ROBOT*";
                break;
            case 32:
                data.color = ccColor3B({ 174, 0, 255 });
                data.text = "*TURNS INTO SPIDER*";
                break;
            case 33:
                data.color = ccColor3B({ 251, 255, 0 });
                data.text = "*TURNS INTO SWING*";
                break;
            case 34:
                data.color = ccColor3B({255, 255, 0});
                data.text = "*USES YELLOW ORB*";
                break;
            case 35:
                data.color = ccColor3B({255, 0, 255});
                data.text = "*USES PINK ORB*";
                break;
            case 36:
                data.color = ccColor3B({ 255, 0, 0 });
                data.text = "*USES RED ORB*";
                break;
            case 37:
                data.color = ccColor3B({ 0, 255, 255 });
                data.text = "*USES BLUE ORB*";
                break;
            case 38:
                data.color = ccColor3B({0, 255, 0});
                data.text = "*USES GREEN ORB*";
                break;
            case 39:
                data.color = ccColor3B({64, 64, 64});
                data.text = "*USES BLACK ORB*";
                break;
            case 40:
                data.color = ccColor3B({255, 255, 255});
                data.text = "*TRIGGERS ORB*";
                break;
            case 43:
                data.color = ccColor3B({ 119, 0, 255 });
                data.text = "*USES SPIDER ORB*";
                break;
            case 45:
                data.color = ccColor3B({ 255, 238, 0 });
                data.text = "*USES YELLOW PAD*";
                break;
            case 46:
                data.color = ccColor3B({ 255, 70, 240 });
                data.text = "*USES PINK PAD*";
                break;
            case 47:
                data.color = ccColor3B({ 255, 53, 17 });
                data.text = "*USES RED PAD*";
                break;
            case 48:
                data.color = ccColor3B({ 0, 217, 255 });
                data.text = "*USES BLUE PAD*";
                break;
            case 49:
                data.color = ccColor3B({ 132, 0, 255 });
                data.text = "*USES SPIDER PAD*";
                break;
            case 57:
                data.color = ccColor3B({ 255, 153, 0 });
                data.text = "*ENTERS DUAL MODE*";
                break;
            case 58:
                data.color = ccColor3B({ 0, 238, 255 });
                data.text = "*EXITS DUAL MODE*";
                break;
            case 62:
                data.color = ccColor3B({ 255, 253, 132 });
                data.text = "*PICKS UP COIN*";
                break;
            case 63:
                data.color = ccColor3B({ 194, 194, 194 });
                data.text = "*PICKS UP ITEM*";
                break;
            case 55:
                data.color = ccColor3B({ 0, 255, 0 });
                data.text = "*TURNS NORMAL SIZE*";
                break;
            case 56:
                data.color = ccColor3B({ 255, 0, 255 });
                data.text = "*TURNS MINI*";
                break;
            default:
                data.color = playerColor;
                data.text = "";
                break;
        }

        return data;
    }

    void gameEventTriggered(GJGameEvent p0, int p1, int p2) {
        GJBaseGameLayer::gameEventTriggered(p0, p1, p2);

        if (!Mod::get()->getSettingValue<bool>("enable-wspeed")) return;

        auto fields = m_fields.self();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float posLimit = 80.f;
        WSpeedFontData fData = getFontData(p0);

        if (!fields->actionFont) return;
        if (fData.text.empty()) return;

        float randX = randomFloat(posLimit * 1.5, winSize.width - posLimit * 1.5);
        float randY = randomFloat(posLimit * 0.5, winSize.height - posLimit * 0.5);

        auto labelShow = CCSequence::create(
            CCFadeIn::create(0.f),
            CCDelayTime::create(0.25f),
            CCEaseOut::create(CCFadeOut::create(0.45f), 2.f),
            nullptr
        );

        auto wobble = CCSequence::create(
            CCEaseInOut::create(CCMoveBy::create(0.35f, {realRandomFloat(1.f, 7.f), realRandomFloat(1.f, 8.f)}), 2.f),
            CCEaseInOut::create(CCMoveBy::create(0.35f, {-realRandomFloat(1.f, 7.f), -realRandomFloat(1.f, 8.f)}), 2.f),
            nullptr
        );

        fields->actionFont->setPosition({randX, randY});
        fields->actionFont->setString(fData.text.c_str(), true);
        fields->actionFont->setColor(fData.color);
        fields->actionFont->stopAllActions();
        fields->actionFont->runAction(labelShow);
        fields->actionFont->runAction(wobble);
    }
};

class $modify(WSpeedPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (!Mod::get()->getSettingValue<bool>("enable-wspeed")) return;

        auto base = GJBaseGameLayer::get();
        if (!base) return;

        auto baseFields = static_cast<WSpeedBaseLayer*>(base)->m_fields.self();

        baseFields->actionFont = CCLabelBMFont::create("*EXISTS*", "WSpeed.fnt"_spr);
        baseFields->actionFont->setOpacity(0);
        baseFields->actionFont->setScale(0.7f);
        
        m_uiLayer->addChild(baseFields->actionFont, 100);
    }

    void levelComplete() {
        PlayLayer::levelComplete();

        if (!Mod::get()->getSettingValue<bool>("enable-wspeed")) return;

        auto base = GJBaseGameLayer::get();
        if (!base) return;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto baseFields = static_cast<WSpeedBaseLayer*>(base)->m_fields.self();

        auto levelCompleteAction = CCSequence::create(
            CCFadeIn::create(0.f),
            CCSpawn::create(
                CCEaseOut::create(CCScaleTo::create(1.75f, 1.1f), 2.5f),
                CCSequence::create(
                    CCDelayTime::create(1.f),
                    CCEaseOut::create(CCFadeOut::create(0.75f), 2.5f),
                    nullptr
                ),
                nullptr
            ),
            nullptr
        );

        auto wobble = CCSequence::create(
            CCEaseInOut::create(CCMoveBy::create(1.1f, {realRandomFloat(1.f, 7.f), realRandomFloat(1.f, 8.f)}), 2.f),
            CCEaseInOut::create(CCMoveBy::create(1.1f, {-realRandomFloat(1.f, 7.f), -realRandomFloat(1.f, 8.f)}), 2.f),
            nullptr
        );

        baseFields->actionFont->setColor(GameManager::sharedState()->colorForIdx(GameManager::sharedState()->getPlayerColor()));
        baseFields->actionFont->setString(fmt::format("*W {} ^*", GJAccountManager::get()->m_username).c_str(), true);
        baseFields->actionFont->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        baseFields->actionFont->stopAllActions();
        baseFields->actionFont->runAction(levelCompleteAction);
        baseFields->actionFont->runAction(wobble);
    }
};