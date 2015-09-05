//
//  GameLayer.h
//  Richer
//
//  Created by Macbook Air on 9/1/15.
//
//

#ifndef __Richer__GameLayer__
#define __Richer__GameLayer__

#include "Richer.h"
#include "DoubleDList.h"
#include "PlayerSprite.h"
#include "LandSprite.h"

class GameLayer : public cocos2d::Layer
{
    std::vector<PlayerSprite *> playerSprites;
    DoubleDList<LandSprite *> landSprites;

    Sprite *pauseBtn;
    Sprite *avatarBtn;
    Sprite *diceBtn;
    cocos2d::ui::Text *dayTxt;
    cocos2d::ui::Text *cashTxt;
    cocos2d::ui::Text *ticketTxt;
    cocos2d::ui::Text *blockTxt;
    cocos2d::ui::Text *bombTxt;
    cocos2d::ui::Text *robotTxt;
    Point prvTouchLoc;
public:
    static cocos2d::Scene* createScene(int fund = 10000);
    static GameLayer *create(int fund = 10000);
    GameLayer();
    ~GameLayer();
    void initWidget(cocos2d::Node *toolNode);
    void initTouchListener();
    void initLandSprite(LandSprite *land, int streetVal, Position p);
    void initMap();
    bool touchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void touchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void touchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    
    void changePOV(Position p);
    DoubleDList<LandSprite *>::DDListIte<LandSprite *> locateLand(Position p);
    void move(int step);
    void updateToolsLayer();
    void nextTurn();
};

#endif /* defined(__Richer__GameLayer__) */