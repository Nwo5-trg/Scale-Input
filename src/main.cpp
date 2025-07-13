#include <Geode/Geode.hpp>
#include <Geode/modify/GJScaleControl.hpp>

using namespace geode::prelude;

enum class ScaleType {
    XY = 0,
    X = 1,
    Y = 2,
    All = 3
};

class $modify (ScaleControl, GJScaleControl) {
    struct Fields {
        CCLabelBMFont* customScaleDefaultLabel;
        CCLabelBMFont* customScaleXLabel;
        CCLabelBMFont* customScaleYLabel;
        TextInput* scaleDefaultInput;
        TextInput* scaleXInput;
        TextInput* scaleYInput;
        CCMenu* defaultShortcutsMenu;
        CCMenu* xShortcutsMenu;
        CCMenu* yShortcutsMenu;
        float shortcutAlignment;
        float lastChangedValueXY = 0.0f;
        float lastChangedValueX = 0.0f;
        float lastChangedValueY = 0.0f;

        // settings
        bool scaleInputEnabled = Mod::get()->getSettingValue<bool>("scale-input-enabled");
        bool scaleHack = Mod::get()->getSettingValue<bool>("scale-hack");
        int64_t maxCharacters = Mod::get()->getSettingValue<int64_t>("scale-max-characters");
        int64_t scaleRounding = Mod::get()->getSettingValue<int64_t>("scale-rounding");
        bool shortcutsEnabled = Mod::get()->getSettingValue<bool>("scale-shortcuts-enabled");
        int64_t shortcutAmount = Mod::get()->getSettingValue<int64_t>("shortcut-amount");
        bool xyShortcutsEnabled = Mod::get()->getSettingValue<bool>("x-y-shortcuts-enabled");
        float shortcutOne = Mod::get()->getSettingValue<double>("shortcut-one");
        float shortcutTwo = Mod::get()->getSettingValue<double>("shortcut-two");
        float shortcutThree = Mod::get()->getSettingValue<double>("shortcut-three");
        float shortcutFour = Mod::get()->getSettingValue<double>("shortcut-four");
        float shortcutFive = Mod::get()->getSettingValue<double>("shortcut-five");
    };

    bool init() {
        if (!GJScaleControl::init()) return false;

        if (m_fields->scaleInputEnabled) {
            auto menu = CCMenu::create();
            menu->setPosition(0.0f, 0.0f);
            menu->setID("scale-label-menu");
            this->addChild(menu);

            auto setupInput = [this, menu] (TextInput* input, CCPoint position, std::string id) {
                input->setPosition(position);
                input->setScale(0.55f);
                input->setFilter("1234567890.-");
                input->setMaxCharCount(m_fields->maxCharacters);
                input->setID(id);
                menu->addChild(input);
            };

            auto scaleDefaultInput = TextInput::create(50.0f, "1.00");
            setupInput(scaleDefaultInput, ccp(35.0f, 28.5f), "scale-default-input");
            scaleDefaultInput->setCallback([this](const std::string& input) {
                if (input.find_first_of("0123456789") != std::string::npos) customScale(std::stof(input), ScaleType::XY);
            });
            m_fields->scaleDefaultInput = scaleDefaultInput;

            auto scaleXInput = TextInput::create(50, "1.00");
            setupInput(scaleXInput, ccp(42.0f, 28.5f), "scale-x-input");
            scaleXInput->setCallback([this](const std::string& input) {
                if (input.find_first_of("0123456789") != std::string::npos) customScale(std::stof(input), ScaleType::X);
            });
            m_fields->scaleXInput = scaleXInput;

            auto scaleYInput = TextInput::create(50.0f, "1.00");
            setupInput(scaleYInput, ccp(42.0f, 88.5f), "scale-y-input");
            scaleYInput->setCallback([this](const std::string& input) {
                if (input.find_first_of("0123456789") != std::string::npos) customScale(std::stof(input), ScaleType::Y);
            });
            m_fields->scaleYInput = scaleYInput;

            auto setupLabel = [menu] (CCLabelBMFont* label, CCPoint position, std::string id) {
                label->setScale(0.6f);
                label->setAnchorPoint(ccp(0.0f, 0.5f));
                label->setPosition(position);
                label->setID(id);
                menu->addChild(label);
            };

            auto scaleDefaultLabel = CCLabelBMFont::create("Scale: ", "bigFont.fnt");
            setupLabel(scaleDefaultLabel, ccp(-49.0f, 30.0f), "scale-default-label");
            m_fields->customScaleDefaultLabel = scaleDefaultLabel;

            auto scaleXLabel = CCLabelBMFont::create("ScaleX: ", "bigFont.fnt");
            setupLabel(scaleXLabel, ccp(-56.0f, 30.0f), "scale-x-label");
            m_fields->customScaleXLabel = scaleXLabel;
            
            auto scaleYLabel = CCLabelBMFont::create("ScaleY: ", "bigFont.fnt");
            setupLabel(scaleYLabel, ccp(-56.0f, 90.0f), "scale-y-label");
            m_fields->customScaleYLabel = scaleYLabel;

            m_scaleLabel->setOpacity(0);
            m_scaleXLabel->setOpacity(0);
            m_scaleYLabel->setOpacity(0);
        }

        if (m_fields->shortcutsEnabled) {
            auto setupShortcutMenu = [&] (CCMenu* menu, std::string id) {
                menu->setLayout(RowLayout::create()
                ->setGap(3.0f)
                ->setGrowCrossAxis(false)
                ->setAxisAlignment(AxisAlignment::Start));
                menu->setContentSize(CCSize(248.75f, 49.0f));
                menu->setScale(0.4f);
                menu->setID(id);
                this->addChild(menu);
            };

            auto defaultShortcutsMenu = CCMenu::create();
            setupShortcutMenu(defaultShortcutsMenu, "default-shortcuts-menu");
            m_fields->defaultShortcutsMenu = defaultShortcutsMenu;

            if (m_fields->xyShortcutsEnabled) {
                auto xShortcutsMenu = CCMenu::create();
                setupShortcutMenu(xShortcutsMenu, "x-shortcuts-menu");
                m_fields->xShortcutsMenu = xShortcutsMenu;

                auto yShortcutsMenu = CCMenu::create();
                setupShortcutMenu(yShortcutsMenu, "y-shortcuts-menu");
                m_fields->yShortcutsMenu = yShortcutsMenu;
            }

            auto makeShortcut = [&] (float value, int buttonNumber, CCMenu* menu, ScaleType type) {
                std::string string = ftofstr(value, 5);
                if (!(string.find_first_of("0123456789") != std::string::npos)) string = "1";

                auto shortcut = CCMenuItemSpriteExtra::create(CircleButtonSprite::create(
                CCLabelBMFont::create(string.c_str(), "bigFont.fnt")), this, menu_selector(ScaleControl::onScaleShortcut));
                shortcut->setID(fmt::format("shortcut-{}", buttonNumber));

                auto valueNode = CCNode::create();
                valueNode->setID(string);
                shortcut->addChild(valueNode);

                auto typeNode = CCNode::create();
                typeNode->setID(std::to_string(static_cast<int>(type)));
                shortcut->addChild(typeNode);

                menu->addChild(shortcut);
                menu->updateLayout();
            };

            std::vector<float> alignments = {40.2f, 30.35f, 20.2f, 10.35f, 0.0f};
            std::vector<float> values = {m_fields->shortcutOne, m_fields->shortcutTwo, 
            m_fields->shortcutThree, m_fields->shortcutFour, m_fields->shortcutFive};

            for (int i = 0; i < m_fields->shortcutAmount; i++) {
                auto value = values[i];
                makeShortcut(value, i + 1, m_fields->defaultShortcutsMenu, ScaleType::XY);
                if (m_fields->xyShortcutsEnabled) {
                    makeShortcut(value, i + 1, m_fields->xShortcutsMenu, ScaleType::X);
                    makeShortcut(value, i + 1, m_fields->yShortcutsMenu, ScaleType::Y);
                }
                m_fields->shortcutAlignment = alignments[i];
            }
        }
        return true;
    }

    void onScaleShortcut(CCObject* sender) {
        float scale = std::strtof(static_cast<CCNode*>(sender)->getChildByType<CCNode>(1)->getID().c_str(), nullptr);
        ScaleType type = static_cast<ScaleType>(std::strtol(static_cast<CCNode*>(sender)->getChildByType<CCNode>(2)->getID().c_str(), nullptr, 10));
        customScale(scale, type);
        this->updateInputValues(scale, scale, type);
    }

    void updateVisibility() { // could be refactored even more (atleast its better than what it used to be) but i dont feel like figuring out how my old code actually works)
        auto fields = m_fields.self();
        if (m_scaleLabel->isVisible()) {
            fields->customScaleDefaultLabel->setVisible(true);
            fields->scaleDefaultInput->setVisible(true);
            fields->defaultShortcutsMenu->setVisible(true);
            fields->customScaleXLabel->setVisible(false);
            fields->scaleXInput->setVisible(false);
            fields->customScaleYLabel->setVisible(false);
            fields->scaleYInput->setVisible(false);
            fields->xShortcutsMenu->setVisible(false);
            fields->yShortcutsMenu->setVisible(false);
            m_scaleLockButton->getParent()->setPosition(ccp(0.0f, 90.0f));
            if (!fields->xyShortcutsEnabled) fields->defaultShortcutsMenu->setPosition(ccp(fields->shortcutAlignment, 60));
            else {
                fields->defaultShortcutsMenu->setPosition(ccp(fields->shortcutAlignment, 60.0f));
                fields->defaultShortcutsMenu->setVisible(true);
                if (fields->shortcutsEnabled && fields->xyShortcutsEnabled) {
                    fields->xShortcutsMenu->setVisible(false);
                    fields->yShortcutsMenu->setVisible(false);
                }
            }
        }
        else {
            fields->customScaleDefaultLabel->setVisible(false);
            fields->scaleDefaultInput->setVisible(false);
            fields->defaultShortcutsMenu->setVisible(false);
            fields->customScaleXLabel->setVisible(true);
            fields->scaleXInput->setVisible(true);
            fields->customScaleYLabel->setVisible(true);
            fields->scaleYInput->setVisible(true);
            fields->xShortcutsMenu->setVisible(true);
            fields->yShortcutsMenu->setVisible(true);
            if (!fields->xyShortcutsEnabled) {
                fields->defaultShortcutsMenu->setPosition(ccp(fields->shortcutAlignment, 120.0f));
                m_scaleLockButton->getParent()->setPosition(ccp(0.0f, 150.0f));
            } else {
                fields->defaultShortcutsMenu->setVisible(false);
                if (fields->shortcutsEnabled && fields->xyShortcutsEnabled) {
                    fields->xShortcutsMenu->setPosition(ccp(fields->shortcutAlignment, 60.0f));
                    fields->xShortcutsMenu->setVisible(true);
                    m_scaleLockButton->getParent()->setPosition(ccp(0.0f, 180.0f));
                    m_sliderY->setPosition(ccp(0.0f, 90.0f));
                    fields->customScaleYLabel->setPosition(ccp(-56.0f, 120.0f));
                    fields->scaleYInput->setPosition(ccp(42.0f, 118.5f));
                    fields->yShortcutsMenu->setPosition(ccp(fields->shortcutAlignment, 150.0f));
                    fields->yShortcutsMenu->setVisible(true);
                }
            }
        }
    }

    void customScale(float scale, ScaleType type) {
        auto editor = EditorUI::get();
        if (scale == 0.0f) scale = 1.0f;
        if (!m_fields->scaleHack) {
            switch (type) {
                case ScaleType::XY: {
                    float currentScale = std::max(m_valueX, m_valueY);
                    if (currentScale == 0.0f) currentScale = 1.0f;
                    editor->scaleXYChanged(scale * (m_valueX / currentScale), scale * (m_valueY / currentScale), m_scaleLocked);
                    m_sliderXY->setValue(valueFromScale(scale));
                    m_valueX = scale * (m_valueX / currentScale);
                    m_valueY = scale * (m_valueY / currentScale);
                    break;
                }
                case ScaleType::X: {
                    editor->scaleXChanged(scale, m_scaleLocked);
                    m_sliderX->setValue(valueFromScale(scale));
                    m_valueX = scale;
                    break;
                }
                case ScaleType::Y: {
                    editor->scaleYChanged(scale, m_scaleLocked);
                    m_sliderY->setValue(valueFromScale(scale));
                    m_valueY = scale;
                    break;
                }
                case ScaleType::All: break;
            }
        } else {
            auto objsArray = editor->getSelectedObjects();
            auto center = editor->getGroupCenter(objsArray, true);
            auto objs = ccArrayToVector<GameObject*>(objsArray);

            float scaleX = FLT_MIN;
            float scaleY = FLT_MIN;
            for (auto obj : objs) {
                float objScaleX = obj->getScaleX();
                float objScaleY = obj->getScaleY();
                if (objScaleX > scaleX) scaleX = objScaleX;
                if (objScaleY > scaleY) scaleY = objScaleY;
            }

            float scaleMultiplier = 0.0f;
            switch (type) {
                case ScaleType::XY: scaleMultiplier = scale / std::max(scaleX, scaleY); break; 
                case ScaleType::X: scaleMultiplier = scale / scaleX; break; 
                case ScaleType::Y: scaleMultiplier = scale / scaleY; break; 
                case ScaleType::All: break;
            }

            for (auto obj : objs) {
                auto pos = obj->getPosition();
                auto offset = pos - center;

                switch (type) {
                    case ScaleType::XY: { // x+y
                        if (!m_scaleLocked) obj->setPosition(center + offset * scaleMultiplier);
                        obj->setScaleX(obj->m_scaleX * scaleMultiplier * (obj->m_isFlipX ? -1 : 1));
                        obj->m_scaleX *= scaleMultiplier;
                        obj->setScaleY(obj->m_scaleY * scaleMultiplier * (obj->m_isFlipY ? -1 : 1));
                        obj->m_scaleY *= scaleMultiplier;
                        break;
                    }
                    case ScaleType::X: { // x
                        if (!m_scaleLocked) obj->setPosition(ccp(center.x + offset.x * scaleMultiplier, pos.y));
                        obj->setScaleX(obj->m_scaleX * scaleMultiplier * (obj->m_isFlipX ? -1 : 1));
                        obj->m_scaleX *= scaleMultiplier;
                        break;
                    }
                    case ScaleType::Y: { // y
                        if (!m_scaleLocked) obj->setPosition(ccp(pos.x, center.y + offset.y * scaleMultiplier));
                        obj->setScaleY(obj->m_scaleY * scaleMultiplier * (obj->m_isFlipY ? -1 : 1));
                        obj->m_scaleY *= scaleMultiplier;
                        break;
                    }
                    case ScaleType::All: break;
                }
            }
            editor->updateButtons();
            editor->updateDeleteButtons();
            editor->updateObjectInfoLabel();
        }
    }
   
    void updateInputValues(float scaleX, float scaleY, ScaleType type) {
        if (m_fields->scaleDefaultInput && (type == ScaleType::XY || type == ScaleType::All)) m_fields->scaleDefaultInput->setString(ftofstr(std::max(scaleX, scaleY), m_fields->scaleRounding));
        if (m_fields->scaleXInput && (type == ScaleType::X || type == ScaleType::All)) m_fields->scaleXInput->setString(ftofstr(scaleX, m_fields->scaleRounding));
        if (m_fields->scaleYInput && (type == ScaleType::Y || type == ScaleType::All)) m_fields->scaleYInput->setString(ftofstr(scaleY, m_fields->scaleRounding));
    }

    void ccTouchMoved(CCTouch* p0, CCEvent* p1) {
        GJScaleControl::ccTouchMoved(p0, p1);
        auto& lastChangedValueXY = m_fields->lastChangedValueXY;
        auto& lastChangedValueX = m_fields->lastChangedValueX;
        auto& lastChangedValueY = m_fields->lastChangedValueY;
        auto sliderXYValue = m_sliderXY->getValue();
        auto sliderXValue = m_sliderX->getValue();
        auto sliderYValue = m_sliderY->getValue();

        if (sliderXYValue != lastChangedValueXY) updateInputValues(m_changedValueX, m_changedValueY, ScaleType::XY);
        if (sliderXValue != lastChangedValueX) updateInputValues(m_changedValueX, m_changedValueY, ScaleType::X);
        if (sliderYValue != lastChangedValueY) updateInputValues(m_changedValueX, m_changedValueY, ScaleType::Y);
        
        lastChangedValueXY = m_sliderXY->getValue();
        lastChangedValueX = m_sliderX->getValue();
        lastChangedValueY = m_sliderY->getValue();
    }

    void loadValues(GameObject* obj, CCArray* objs, gd::unordered_map<int, GameObjectEditorState>& states) {
        GJScaleControl::loadValues(obj, objs, states);
        if (m_fields->scaleInputEnabled) this->updateInputValues(m_valueX, m_valueY, ScaleType::All);
        if (m_fields->scaleInputEnabled || m_fields->shortcutsEnabled) this->updateVisibility();
    }

    std::string ftofstr(float num, int rounding) {
        auto str = numToString(num, rounding);
        auto end = str.find_last_not_of('0');
        if (end != std::string::npos) str.erase(end + 1);
        if (!str.empty() && str.back() == '.') str.pop_back();
        return str;
    }
};