#include "ui/CustomCharacterColorPage.hpp"

using namespace geode::prelude;

CustomCharacterColorPage* CustomCharacterColorPage::customCreate() {
    auto settings = Settings::sharedInstance();

    // i want to properly inherit but everything except this crashes, so: FUCK IT, WE BALL.
    auto self = static_cast<CustomCharacterColorPage*>(CharacterColorPage::create());

    // kill all children brutally, no mercy
    self->removeAllChildrenWithCleanup(true);

    if (!self || !self->init()) {
        log::error("failed to create CustomCharacterColorPage");
        return nullptr;
    }

    CCMenu* menu = self->m_buttonMenu;
    
    if (menu == nullptr) {
        log::error("didn't find color menu");
        return self;
    }

    // fix a crash that occurs when clicking the x button manually
    auto x_button = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildren()->objectAtIndex(0));
    if (x_button) {
        x_button->m_pfnSelector = menu_selector(CustomCharacterColorPage::close);
    }

    int buttons_found = 0;

    for (int i = 0; i < menu->getChildrenCount(); i++) {
        auto child = menu->getChildren()->objectAtIndex(i);
        auto node = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
        if (!node) {
            continue;
        }

        for (int j = 0; j < node->getChildrenCount(); j++) {
            auto node_child = node->getChildren()->objectAtIndex(j);
            if (typeinfo_cast<ColorChannelSprite*>(node_child)) {
                node->m_pfnSelector = menu_selector(CustomCharacterColorPage::onColorClicked);
            }
            if (typeinfo_cast<ButtonSprite*>(node_child)) {
                if (buttons_found > 2) {
                    // this is mainly here to remove the "Rand" button added by Capeling's randomizer mod
                    log::warn("found too many buttons, removing");
                    node->removeFromParentAndCleanup(true);
                    continue;
                }
                switch (buttons_found) {
                    case 0:
                        //log::debug("found primary button");
                        settings->m_button_primary_color = node;
                        break;
                    case 1:
                        //log::debug("found secondary button");
                        settings->m_button_secondary_color = node;
                        break;
                    case 2:
                        //log::debug("found glow button");
                        settings->m_button_glow_color = node;
                        break;
                    default:
                        break;
                }

                buttons_found++;

                node->m_pfnSelector = menu_selector(CustomCharacterColorPage::onColorTypeButtonClicked);
            }
        }
    }

    if (buttons_found < 3) {
        log::error("DID NOT FIND ALL BUTTONS");
        return self;
    }

    // END
    // murder 2nd child from self if it exists (added by an icon hack mod, using it crashes the game)
    if (self->getChildrenCount() > 1) {
        self->removeChild(static_cast<CCNode*>(self->getChildren()->objectAtIndex(1)));
    }

    // gotta catch 'em all!
    if (self->loadSimpsAndSelectionSprites()) {
        // find the ship button BEFORE all other player buttons are added
        auto ship_player = findFirstChildRecursive<SimplePlayer>(menu, [](auto node) {
            return typeinfo_cast<SimplePlayer*>(node) != nullptr;
        });
        settings->m_player_ship = ship_player;
        auto ship_button = typeinfo_cast<CCMenuItemSprite*>(ship_player->getParent());
        ship_button->setTag(SHIP);
        ship_button->m_pfnSelector = menu_selector(CustomCharacterColorPage::onPlayerClicked);

        // cube button
        auto cube_button = self->createPlayerButton(settings->m_player_cube, CUBE);
        menu->addChild(cube_button);

        auto ball_button = self->createPlayerButton(settings->m_player_ball, BALL);
        menu->addChild(ball_button);

        auto ufo_button = self->createPlayerButton(settings->m_player_ufo, UFO);
        menu->addChild(ufo_button);

        auto wave_button = self->createPlayerButton(settings->m_player_wave, WAVE);
        menu->addChild(wave_button);

        auto robot_button = self->createPlayerButton(settings->m_player_robot, ROBOT);
        menu->addChild(robot_button);

        auto spider_button = self->createPlayerButton(settings->m_player_spider, SPIDER);
        menu->addChild(spider_button);

        auto swing_button = self->createPlayerButton(settings->m_player_swing, SWING);
        menu->addChild(swing_button);

        float button_width = 33.0f;
        auto cube_toggle_button = self->createGameModeButton(CUBE, {button_width, -7});
        menu->addChild(cube_toggle_button);

        auto ship_toggle_button = self->createGameModeButton(SHIP, {button_width * 2, -7});
        menu->addChild(ship_toggle_button);

        auto ball_toggle_button = self->createGameModeButton(BALL, {button_width * 3, -7});
        menu->addChild(ball_toggle_button);

        auto ufo_toggle_button = self->createGameModeButton(UFO, {button_width * 4, -7});
        menu->addChild(ufo_toggle_button);

        auto wave_toggle_button = self->createGameModeButton(WAVE, {button_width * 5, -7});
        menu->addChild(wave_toggle_button);

        auto robot_toggle_button = self->createGameModeButton(ROBOT, {button_width * 6, -7});
        menu->addChild(robot_toggle_button);

        auto spider_toggle_button = self->createGameModeButton(SPIDER, {button_width * 7, -7});
        menu->addChild(spider_toggle_button);

        auto swing_toggle_button = self->createGameModeButton(SWING, {button_width * 8, -7});
        menu->addChild(swing_toggle_button);

        // sprite to show current player
        auto current_gamemode_sprite = CCSprite::createWithSpriteFrameName(TEXTURE_SELECTED_FRAME);
        current_gamemode_sprite->setContentSize({32, 32});
        current_gamemode_sprite->setScale(1.15f);
        self->m_mainLayer->addChild(current_gamemode_sprite);
        settings->m_current_gamemode_sprite = current_gamemode_sprite;

        self->updateUI();
    } else {
        log::error("failed to load icons and selection sprites (this should never happen)");
    }

    //self->autorelease();
    //log::debug("CustomCharacterColorPage::create returning self");
    return self;
}

CCMenuItemSpriteExtra* CustomCharacterColorPage::createPlayerButton(SimplePlayer* player, GameMode game_mode) {
    auto player_pos = player->getParent()->convertToWorldSpaceAR(player->getPosition());
    player->removeFromParent();

    auto button = CCMenuItemSpriteExtra::create(
        player,
        this,
        menu_selector(CustomCharacterColorPage::onPlayerClicked)
    );
    button->setContentSize({30, 30});
    player->setPosition({15, 15});

    button->setPosition(this->m_buttonMenu->convertToNodeSpaceAR(player_pos));
    button->setTag(game_mode);

    return button;
}

CCMenuItemSpriteExtra* CustomCharacterColorPage::createGameModeButton(GameMode game_mode, CCPoint position) {
    auto inner_button = ButtonSprite::create(this->getGameModeName(game_mode).c_str(), 16, true, "bigFont.fnt", TEXTURE_BUTTON_ENABLED, 20.0f, 2.0f);
    auto button = CCMenuItemSpriteExtra::create(
        inner_button,
        this,
        menu_selector(CustomCharacterColorPage::onGameModeToggleButtonClicked)
    );

    switch (game_mode) {
        case CUBE:
            Settings::sharedInstance()->m_button_cube = inner_button;
            break;
        case SHIP:
            Settings::sharedInstance()->m_button_ship = inner_button;
            break;
        case BALL:
            Settings::sharedInstance()->m_button_ball = inner_button;
            break;
        case UFO:
            Settings::sharedInstance()->m_button_ufo = inner_button;
            break;
        case WAVE:
            Settings::sharedInstance()->m_button_wave = inner_button;
            break;
        case ROBOT:
            Settings::sharedInstance()->m_button_robot = inner_button;
            break;
        case SPIDER:
            Settings::sharedInstance()->m_button_spider = inner_button;
            break;
        case SWING:
            Settings::sharedInstance()->m_button_swing = inner_button;
            break;
        default:
            break;
    }

    auto settings = Settings::sharedInstance();

    button->setTag(game_mode);
    button->setPosition(position);
    return button;
}

bool CustomCharacterColorPage::loadSimpsAndSelectionSprites() {
    auto meow = this->m_mainLayer;
    int simps_found = 0;
    int sprites_found = 0;
    bool found_all = false;

    auto settings = Settings::sharedInstance();

    for (int i = 0; i < meow->getChildrenCount(); i++) {
        auto simple_player = typeinfo_cast<SimplePlayer*>(meow->getChildren()->objectAtIndex(i));
        auto sprite = typeinfo_cast<CCSprite*>(meow->getChildren()->objectAtIndex(i));

        if (simple_player) {
            switch (simps_found) {
                case 0:
                    //log::debug("found cube");                   
                    settings->m_player_cube = simple_player;
                    break;
                case 1: // this is ball, because ship is inside the menu and not in the layer directly
                    //log::debug("found balls");
                    settings->m_player_ball = simple_player;
                    break;
                case 2:
                    //log::debug("found ufo");
                    settings->m_player_ufo = simple_player;
                    break;
                case 3:
                    //log::debug("found wave");
                    settings->m_player_wave = simple_player;
                    break;
                case 4:
                    //log::debug("found robot");
                    settings->m_player_robot = simple_player;
                    break;
                case 5:
                    //log::debug("found spider");
                    settings->m_player_spider = simple_player;
                    break;
                case 6:
                    //log::debug("found swing");
                    settings->m_player_swing = simple_player;
                    break;
                default:
                    break;
            }

            simps_found++;
        }

        // SimplePlayer is a subclass of CCSprite
        if (sprite && !simple_player) {
            //log::debug("found sprite");

            switch (sprites_found) {
                case 0:
                    settings->m_current_color_primary_sprite = sprite;
                    break;
                case 1:
                    settings->m_current_color_secondary_sprite = sprite;
                    break;
                case 2:
                    settings->m_current_color_glow_sprite = sprite;
                    break;
                default:
                    break;
            }

            sprites_found++;
        }

        found_all = simps_found == 7 && sprites_found == 3;

        if (found_all) {
            break;
        }
    }

    return found_all;
}

void CustomCharacterColorPage::close(CCObject* sender) {
    this->removeAllChildrenWithCleanup(true);
    // prevent crash, hacky but works for now
    // a user is probably not going to close this menu so often that this would make any difference
    this->retain();
    
    this->removeFromParentAndCleanup(true);
}

void CustomCharacterColorPage::updateUI() {
    auto settings = Settings::sharedInstance();
    auto gameManager = GameManager::get();

    if(settings->m_player_cube) {
        int col1 = settings->m_override_cube ? settings->m_cube_override  : settings->m_defaultColor;
        int col2 = settings->m_override_cube ? settings->m_cube_override2 : settings->m_defaultColor2;

        settings->m_player_cube->setColor(gameManager->colorForIdx(col1));
        settings->m_player_cube->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_ship) {
        int col1 = settings->m_override_ship ? settings->m_ship_override  : settings->m_defaultColor;
        int col2 = settings->m_override_ship ? settings->m_ship_override2 : settings->m_defaultColor2;

        settings->m_player_ship->setColor(gameManager->colorForIdx(col1));
        settings->m_player_ship->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_ball) {
        int col1 = settings->m_override_ball ? settings->m_ball_override  : settings->m_defaultColor;
        int col2 = settings->m_override_ball ? settings->m_ball_override2 : settings->m_defaultColor2;

        settings->m_player_ball->setColor(gameManager->colorForIdx(col1));
        settings->m_player_ball->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_ufo) {
        int col1 = settings->m_override_ufo ? settings->m_ufo_override  : settings->m_defaultColor;
        int col2 = settings->m_override_ufo ? settings->m_ufo_override2 : settings->m_defaultColor2;

        settings->m_player_ufo->setColor(gameManager->colorForIdx(col1));
        settings->m_player_ufo->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_wave) {
        int col1 = settings->m_override_wave ? settings->m_wave_override  : settings->m_defaultColor;
        int col2 = settings->m_override_wave ? settings->m_wave_override2 : settings->m_defaultColor2;

        settings->m_player_wave->setColor(gameManager->colorForIdx(col1));
        settings->m_player_wave->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_robot) {
        int col1 = settings->m_override_robot ? settings->m_robot_override  : settings->m_defaultColor;
        int col2 = settings->m_override_robot ? settings->m_robot_override2 : settings->m_defaultColor2;

        settings->m_player_robot->setColor(gameManager->colorForIdx(col1));
        settings->m_player_robot->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_spider) {
        int col1 = settings->m_override_spider ? settings->m_spider_override  : settings->m_defaultColor;
        int col2 = settings->m_override_spider ? settings->m_spider_override2 : settings->m_defaultColor2;

        settings->m_player_spider->setColor(gameManager->colorForIdx(col1));
        settings->m_player_spider->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_player_swing) {
        int col1 = settings->m_override_swing ? settings->m_swing_override  : settings->m_defaultColor;
        int col2 = settings->m_override_swing ? settings->m_swing_override2 : settings->m_defaultColor2;

        settings->m_player_swing->setColor(gameManager->colorForIdx(col1));
        settings->m_player_swing->setSecondColor(gameManager->colorForIdx(col2));
    }

    if(settings->m_current_color_primary_sprite) {
        if (settings->m_current_mode == NONE) {
            settings->m_current_color_primary_sprite->setVisible(false);
        } else {
            this->updateColorSelectionSprite(settings->m_current_color_primary_sprite, PRIMARY);
        }
    }

    if(settings->m_current_color_secondary_sprite) {
        if (settings->m_current_mode == NONE) {
            settings->m_current_color_secondary_sprite->setVisible(false);
        } else {
            this->updateColorSelectionSprite(settings->m_current_color_secondary_sprite, SECONDARY);
        }
    }

    if(settings->m_current_color_glow_sprite) {
        // not supported yet
        settings->m_current_color_glow_sprite->setVisible(false);
    }

    if (settings->m_button_cube) {
        settings->m_button_cube->updateBGImage(settings->m_override_cube ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_ship) {
        settings->m_button_ship->updateBGImage(settings->m_override_ship ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_ball) {
        settings->m_button_ball->updateBGImage(settings->m_override_ball ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_ufo) {
        settings->m_button_ufo->updateBGImage(settings->m_override_ufo ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_wave) {
        settings->m_button_wave->updateBGImage(settings->m_override_wave ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_robot) {
        settings->m_button_robot->updateBGImage(settings->m_override_robot ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_spider) {
        settings->m_button_spider->updateBGImage(settings->m_override_spider ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }
    if (settings->m_button_swing) {
        settings->m_button_swing->updateBGImage(settings->m_override_swing ? TEXTURE_BUTTON_ENABLED : TEXTURE_BUTTON_DISABLED);
    }

    if (settings->m_current_gamemode_sprite) {
        this->updateGameModeSelectionSprite();
    }
}

void CustomCharacterColorPage::updateColorSelectionSprite(CCSprite* sprite, ColorType type) {
    auto settings = Settings::sharedInstance();

    int color = 0;

    switch (settings->m_current_mode) {
        case CUBE:
            color = type == PRIMARY ? settings->m_cube_override : settings->m_cube_override2;
            break;
        case SHIP:
            color = type == PRIMARY ? settings->m_ship_override : settings->m_ship_override2;
            break;
        case BALL:
            color = type == PRIMARY ? settings->m_ball_override : settings->m_ball_override2;
            break;
        case UFO:
            color = type == PRIMARY ? settings->m_ufo_override : settings->m_ufo_override2;
            break;
        case WAVE:
            color = type == PRIMARY ? settings->m_wave_override : settings->m_wave_override2;
            break;
        case ROBOT:
            color = type == PRIMARY ? settings->m_robot_override : settings->m_robot_override2;
            break;
        case SPIDER:
            color = type == PRIMARY ? settings->m_spider_override : settings->m_spider_override2;
            break;
        case SWING:
            color = type == PRIMARY ? settings->m_swing_override : settings->m_swing_override2;
            break;
        default:
            break;
    }

    if (type != settings->m_current_color_type) {
        sprite->setColor(ccc3(50, 50, 50));
    } else {
        sprite->setColor(ccc3(255, 255, 255));
    }

    auto target_pos = this->m_mainLayer->convertToNodeSpaceAR(this->getPositionOfColor(color));
    sprite->setPosition(target_pos);
    sprite->setVisible(true);
}

void CustomCharacterColorPage::updateGameModeSelectionSprite() {
    auto settings = Settings::sharedInstance();

    auto sprite = settings->m_current_gamemode_sprite;
    if (settings->m_current_mode == NONE) {
        sprite->setVisible(false);
        return;
    } else {
        sprite->setVisible(true);
    }

    SimplePlayer* player = nullptr;

    switch (settings->m_current_mode) {
        case CUBE:
            player = settings->m_player_cube;
            break;
        case SHIP:
            player = settings->m_player_ship;
            break;
        case BALL:
            player = settings->m_player_ball;
            break;
        case UFO:
            player = settings->m_player_ufo;
            break;
        case WAVE:
            player = settings->m_player_wave;
            break;
        case ROBOT:
            player = settings->m_player_robot;
            break;
        case SPIDER:
            player = settings->m_player_spider;
            break;
        case SWING:
            player = settings->m_player_swing;
            break;
        default:
            break;
    }

    if (!player) {
        log::error("failed to find player for current mode");
        return;
    }

    // all simpleplayers (except ship) are initially children of m_mainLayer,
    // but since we moved them to buttons inside the menu, we need to convert the position
    auto pos = this->m_mainLayer->convertToNodeSpaceAR(this->m_buttonMenu->convertToWorldSpaceAR(player->getParent()->getPosition()));
    
    // pos is slightly off, so we need to fix it
    auto fixed_pos = ccpAdd(pos, ccp(-3.5, -3));

    sprite->setPosition(fixed_pos);
}

CCPoint CustomCharacterColorPage::getPositionOfColor(int color_id) {
    auto child = findFirstChildRecursive<CCMenuItemSpriteExtra>(this->m_buttonMenu, [color_id](auto node) {
        return typeinfo_cast<ColorChannelSprite*>(node->getChildren()->objectAtIndex(0)) && node->getTag() == color_id;
    });
    if (!child) {
        log::error("failed to find child with tag: {}", color_id);
        return {0, 0};
    } else {
        return this->m_buttonMenu->convertToWorldSpaceAR(child->getPosition());
    }
}

void CustomCharacterColorPage::onColorClicked(CCObject* sender) {
    auto node = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!node) {
        log::error("sender is not a CCMenuItemSpriteExtra");
        return;
    }
    int tag = node->getTag();

    auto settings = Settings::sharedInstance();

    settings->setOverrideColor(settings->m_current_mode, tag, settings->m_current_color_type);

    this->updateUI();
}

void CustomCharacterColorPage::onPlayerClicked(CCObject* sender) {
    auto node = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!node) {
        log::error("sender is not a CCMenuItemSpriteExtra");
        return;
    }
    int tag = node->getTag();

    if (tag < 1 || tag > 8) {
        log::error("invalid tag: {}", tag);
        return;
    }

    Settings::sharedInstance()->m_current_mode = static_cast<GameMode>(tag);

    this->updateUI();
}

void CustomCharacterColorPage::onColorTypeButtonClicked(CCObject* sender) {
    auto node = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!node) {
        log::error("sender is not a CCMenuItemSpriteExtra");
        return;
    }
    int tag = node->getTag();

    if (tag < 0 || tag > 2) {
        log::error("invalid tag: {}", tag);
        return;
    }

    auto settings = Settings::sharedInstance();

    settings->m_current_color_type = static_cast<ColorType>(tag);

    // set buttons
    auto primary_button = settings->m_button_primary_color;
    auto secondary_button = settings->m_button_secondary_color;
    auto glow_button = settings->m_button_glow_color;

    switch (settings->m_current_color_type) {
        case PRIMARY:
            static_cast<ButtonSprite*>(primary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_ENABLED);
            static_cast<ButtonSprite*>(secondary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            static_cast<ButtonSprite*>(glow_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            break;
        case SECONDARY:
            static_cast<ButtonSprite*>(primary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            static_cast<ButtonSprite*>(secondary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_ENABLED);
            static_cast<ButtonSprite*>(glow_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            break;
        case GLOW:
            static_cast<ButtonSprite*>(primary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            static_cast<ButtonSprite*>(secondary_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_DISABLED);
            static_cast<ButtonSprite*>(glow_button->getChildren()->objectAtIndex(0))->updateBGImage(TEXTURE_BUTTON_ENABLED);
            break;
    }

    this->updateUI();
}

void CustomCharacterColorPage::onGameModeToggleButtonClicked(CCObject* sender) {
    int tag = sender->getTag();

    if (tag < 1 || tag > 8) {
        log::error("invalid tag: {}", tag);
        return;
    }

    auto game_mode = static_cast<GameMode>(tag);

    Settings::sharedInstance()->toggleOverride(game_mode);
    updateUI();
}

// todo: move this to a utils class
std::string CustomCharacterColorPage::getGameModeName(GameMode mode) {
    switch (mode) {
        case CUBE:
            return "cube";
        case SHIP:
            return "ship";
        case BALL:
            return "ball";
        case UFO:
            return "ufo";
        case WAVE:
            return "wave";
        case ROBOT:
            return "robot";
        case SPIDER:
            return "spider";
        case SWING:
            return "swing";
        default:
            return "none";
    }
}