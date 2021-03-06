﻿//
//  GameLayer.cpp
//  Richer
//
//  Created by Macbook Air on 9/1/15.
//
//

#include "GameLayer.h"
#include "ShopLayer.h"
#include "PauseLayer.h"
#include "OverLayer.h"

USING_NS_CC;

using namespace cocostudio::timeline;

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Initalization

Scene* GameLayer::createScene(int fund)
{
    auto scene = Scene::create();

    // holds the back ground
    auto bgLayer = Layer::create();
    auto bgNode = CSLoader::createNode("BgLayer.csb");
    bgLayer->addChild(bgNode);
    scene->addChild(bgLayer, 1);

    // holds the buttons, labels for day, cash and such
    auto toolsLayer = Layer::create();
    auto toolNode = CSLoader::createNode("ToolsLayer.csb");
    toolsLayer->addChild(toolNode);
    scene->addChild(toolsLayer, 3);

    auto layer = GameLayer::create(fund);
    layer->initWidget(toolNode);
    layer->updateToolsLayer();
    scene->addChild(layer, 2);

    return scene;
}

GameLayer *GameLayer::create(int fund)
{
    GameLayer *ret = new (std::nothrow) GameLayer();

    // init players
    for(int i = 0; i < pnum.size(); i++) {
        PlayerSprite *player = PlayerSprite::create(pnum[i], fund);
        player->p = Position(0, MAP_ROW-1);
        ret->playerSprites.push_back(player);
        player->setPosition(player->p.toRealPos());
        ret->addChild(player, (int)pnum.size()-i+4);
    }

    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
}

GameLayer::GameLayer()
{
    initEventListener();
    addObserv();
    initMap();
    isMoving = false;
    isPlantingWhat = ITEM_KINDS;
    notice = NULL;
    ask = NULL;
    //    tmpLabel = NULL;
}
GameLayer::~GameLayer(){}

void GameLayer::initWidget(Node *toolNode) {
    this->pauseBtn = static_cast<Sprite*>( toolNode->getChildByTag(19) );
    this->diceBtn = static_cast<Sprite*>( toolNode->getChildByTag(12) );
    this->avatarBtn = static_cast<Sprite*>( toolNode->getChildByTag(10) );

    this->blockBtn = dynamic_cast<ui::Button*>(toolNode->getChildByName("block"));
    this->bombBtn = dynamic_cast<ui::Button*>(toolNode->getChildByName("bomb"));
    this->robotBtn = dynamic_cast<ui::Button*>(toolNode->getChildByName("robot"));
    blockBtn->addTouchEventListener(CC_CALLBACK_2(GameLayer::blockBtnListener, this));
    bombBtn->addTouchEventListener(CC_CALLBACK_2(GameLayer::bombBtnListener, this));
    robotBtn->addTouchEventListener(CC_CALLBACK_2(GameLayer::robotBtnListener, this));

    this->dayTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("day"));
    this->cashTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("money"));
    this->ticketTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("ticket"));
    this->blockTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("block_txt"));
    this->bombTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("bomb_txt"));
    this->robotTxt = dynamic_cast<ui::Text*>(toolNode->getChildByName("robot_txt"));
}
void GameLayer::initEventListener() {
    auto eventDispatcher = Director::getInstance()->getEventDispatcher();
    auto touchlistener = EventListenerTouchOneByOne::create();
    touchlistener->setSwallowTouches(true);
    touchlistener->onTouchBegan = CC_CALLBACK_2(GameLayer::touchBegan, this);
    touchlistener->onTouchMoved = CC_CALLBACK_2(GameLayer::touchMoved, this);
    touchlistener->onTouchEnded = CC_CALLBACK_2(GameLayer::touchEnded, this);
    eventDispatcher->addEventListenerWithFixedPriority(touchlistener, 1);
}
void GameLayer::addObserv() {
    NotificationCenter::getInstance()->addObserver(this, callfuncO_selector(GameLayer::shopCallBack), "shopCallback", NULL);
    NotificationCenter::getInstance()->addObserver(this, callfuncO_selector(GameLayer::defaultCallBack), "defaultCallback", NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game Map

void GameLayer::initMap() {
    // land type[]
    int lt[MAP_COL];
    for(int i = 0; i < MAP_COL; i++) {
        lt[i] = LTYPE_UNOCCUPIED;
    }
    lt[0]=LTYPE_NOTHING; lt[14]=LTYPE_HOSPITAL; lt[MAP_COL-1]=LTYPE_SHOP;
//    LandSprite *_land = LandSprite::create();
//    initLandSprite(_land, 0, Position(0, MAP_ROW-1));
    for(int i = 0, y = MAP_ROW-1; i < MAP_COL; i++) {
        LandSprite *land = NULL;
        land = LandSprite::create(lt[i]);
        initLandSprite(land, 200, Position(i, y));
    }

    for(int x = MAP_COL-1, j = MAP_ROW-2; j > 0; j--) {
        LandSprite *land = NULL;
        land = LandSprite::create(LTYPE_UNOCCUPIED);
        initLandSprite(land, 500, Position(x, j));
    }

    for(int i = 0; i < MAP_COL; i++) {
        lt[i] = LTYPE_UNOCCUPIED;
    }
    lt[0] = LTYPE_MAGIC; lt[14]=LTYPE_PRISON; lt[MAP_COL-1]=LTYPE_GIFT;
    for(int i = MAP_COL-1, y = 0; i > -1; i--) {
        LandSprite *land  = NULL;
        land = LandSprite::create(lt[i]);
        initLandSprite(land, 300, Position(i, y));
    }

    int ld[MAP_ROW] = {0, 20, 80, 100, 40, 80, 60, 0};
    for(int x = 0, j = 1; j < MAP_ROW-1; j++) {
        LandSprite *land  = NULL;
        land = LandSprite::create(LTYPE_MINE);
        initLandSprite(land, 0, Position(x, j));
        land->data = ld[j];
    }
}

// helper method for initMap()
void GameLayer::initLandSprite(LandSprite *land, int streetVal, Position p) {
    land->setUp(streetVal, p);
    this->landSprites.pushFromHead(land);
    this->addChild(land, 3);
}

void GameLayer::changePOV(Position p) {
    // Relative position to left bottom of the screen, (1, 2)
    this->setPosition(-tileSiz * p.x, tileSiz * (MAP_ROW - 1 - p.y));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game Logic

void GameLayer::transfer(int src, int dst, int amout) {
    playerSprites[getTurnWithWho(src)]->cash -= amout;
    playerSprites[getTurnWithWho(dst)]->cash += amout;
    
    stringHelper.str("");
    stringHelper << "交租金" << amout << "，不能露宿街头，必须租房子住，破产就破产！";
    notifyPlayer(stringHelper.str());
    
    if(playerSprites[getTurnWithWho(src)]->cash < 0)
        brokeProcedure(src);
}
void GameLayer::brokeProcedure(int who) {
    // if there is only two player left and one of them tis broke, then the other must be the winner
    if(playerSprites.size() == 2) {
        gameOver(who);
        return;
    }
    
    notifyPlayer("你破产啦！！！！！房子家产自动消失！！！！！还有人！！！");
    int _turn = getTurnWithWho(who);
    PlayerSprite *player = playerSprites[_turn];
    Texture2D* texture = Director::getInstance()->getTextureCache()->addImage("unoccupied.png");
    for(int i = 0; i < player->properties.size(); i++) {
        LandSprite *land = player->properties[i];
        land->type = LTYPE_UNOCCUPIED;
        land->data = 0;
        land->setTexture(texture);
    }
    
    // delete player from pnum and playerSprites
    int j = 0;
    for(std::vector<PlayerSprite *>::iterator i = playerSprites.begin();
        i != playerSprites.end(); j++) {
        if(j == _turn) {
            (*i)->removeFromParent();
            i = playerSprites.erase(i);
            break;
        }
        i++;
    }
    j = 0;
    for(std::vector<int>::iterator i = pnum.begin();
        i != pnum.end(); j++) {
        if(j == _turn) {
            i = pnum.erase(i);
            break;
        }
        i++;
    }
    updateToolsLayer();
}
void GameLayer::move(int step) {
    DoubleDList<LandSprite *>::DDListIte<LandSprite *> cIter = locateLand(playerSprites[turn]->p);
    Vector< FiniteTimeAction * > arrayOfActions;
    if(playerSprites[turn]->facing == FACING_CLK) {
        for(int i = 0; i < step; i++) {
            cIter.moveBack();
            arrayOfActions.pushBack(MoveTo::create(1/6.0, cIter.getCurrent()->p.toRealPos()));
        }
    }
    else {
        for(int i = 0; i < step; i++) {
            cIter.moveFront();
            arrayOfActions.pushBack(MoveTo::create(1/6.0, cIter.getCurrent()->p.toRealPos()));
        }
    }
    arrayOfActions.pushBack(CallFunc::create(CC_CALLBACK_0(GameLayer::moveAnimCallback, this)));
    playerSprites[turn]->p = cIter.getCurrent()->p;
    changePOV(cIter.getCurrent()->p);
    isMoving = true;
    playerSprites[turn]->runAction(Sequence::create(arrayOfActions));
}
void GameLayer::moveAnimCallback() {
    isMoving = false;
    checkIn();
}

bool GameLayer::checkOut() {
    int status = playerSprites[turn]->status;
    bool ret = true;
    if(status == STATUS_NORM) ret = true;
    if(status / STATUS_MONEYGOD % 10) {
        playerSprites[turn]->status -= STATUS_MONEYGOD;
        ret = true;
    }
    if(status / STATUS_INJURED % 10) {
        playerSprites[turn]->status -= STATUS_INJURED;
        ret = false;
    }
    if(status / STATUS_INPRISON % 10) {
        playerSprites[turn]->status -= STATUS_INPRISON;
        ret = false;
    }
    return ret;
}
void GameLayer::checkIn() {
    LandSprite *land = locateLand(playerSprites[turn]->p).getCurrent();
    switch(land->type) {
        case LTYPE_UNOCCUPIED:
            // can't afford
            if(playerSprites[turn]->cash < land->streetVal)
                break;
            // ask if buy
            stringHelper.str("");
            stringHelper << "买不买，钩还是叉，一口价" << land->streetVal << "？";
            tag = TAG_PURCHASE;
            askPlayer(stringHelper.str());
            return;
        case LTYPE_SHOP:
            // not enough ticket
            if(playerSprites[turn]->ticket < itemCost[ITEM_ROBOT])
                break;
            goShop();
            return;
        case LTYPE_GIFT:
            // TODO
            break;
        case LTYPE_MAGIC:
            // wait for client
            break;
        case LTYPE_HOSPITAL:
        case LTYPE_PRISON: break;
        case LTYPE_MINE:
            playerSprites[turn]->ticket += land->data;
            stringHelper.str("");
            stringHelper << "辛苦搬砖一天一夜，获得" << land->data << "点券！";
            notifyPlayer(stringHelper.str());
            break;
        default:
            // Land type left are: lv1, lv2, maxlv
            if(land->owner != pnum[turn]) {
                int amout = land->data;
                if(playerSprites[turn]->status / STATUS_MONEYGOD % 10)
                    amout/=2;
                transfer(pnum[turn], land->owner, amout);
            }
            else if(land->type == LTYPE_MAXLV) break;
            else {
                // not enough cash for the level-up
                if(playerSprites[turn]->cash < land->streetVal)
                    break;
                stringHelper.str("");
                stringHelper << "就说升不升级，升级牛逼，一口价" << land->streetVal << "！";
                tag = TAG_LEVELUP;
                askPlayer(stringHelper.str());
                return;
            }
    }
    nextTurn();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Use of Game Item

// TODO
void GameLayer::blockBtnListener(cocos2d::Ref *sender, ui::Widget::TouchEventType type) {
    if(playerSprites[turn]->items[ITEM_BLOCK] == 0) return;
    
}
void GameLayer::bombBtnListener(cocos2d::Ref *sender, ui::Widget::TouchEventType type) {
    if(playerSprites[turn]->items[ITEM_BOMB] == 0) return;
}
void GameLayer::robotBtnListener(cocos2d::Ref *sender, ui::Widget::TouchEventType type) {
    if(playerSprites[turn]->items[ITEM_ROBOT] == 0) return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game Logic Helper Methods

int GameLayer::getTurnWithWho(int who) {
    for(int i = 0; i < playerSprites.size(); i++) {
        if(pnum[i] == who)
            return i;
    }
    return 0;
}

DoubleDList<LandSprite *>::DDListIte<LandSprite *> GameLayer::locateLand(Position p) {
    DoubleDList<LandSprite *>::DDListIte<LandSprite *> iter = landSprites.headIte();
    LandSprite *tmp = iter.getCurrent();
    if(iter.getCurrent() == NULL) return NULL;
    do{
        if(iter.getCurrent()->p.isEqual(p))
            return iter;
        iter.moveBack();
    } while(iter.getCurrent() != tmp);
    return NULL;
}

void GameLayer::nextTurn() {
    if(++turn == pnum.size()) {
        day++;
        stringHelper.str("");
        stringHelper << "第" << day << "天";
        dayTxt->setString(stringHelper.str());
        turn = 0;
    }
    updateToolsLayer();
}

void GameLayer::updateToolsLayer() {
    Texture2D* texture = Director::getInstance()->getTextureCache()->addImage(pavatar[pnum[turn]]);
    avatarBtn->setTexture(texture);
    stringHelper.str("");
    stringHelper << "金钱：" << playerSprites[turn]->cash;
    cashTxt->setString(stringHelper.str());
    stringHelper.str("");
    stringHelper << "点数：" << playerSprites[turn]->ticket;
    ticketTxt->setString(stringHelper.str());
    stringHelper.str("");
    stringHelper << "x" << playerSprites[turn]->items[ITEM_BLOCK];
    blockTxt->setString(stringHelper.str());
    stringHelper.str("");
    stringHelper << "x" << playerSprites[turn]->items[ITEM_BOMB];
    bombTxt->setString(stringHelper.str());
    stringHelper.str("");
    stringHelper << "x" << playerSprites[turn]->items[ITEM_ROBOT];
    robotTxt->setString(stringHelper.str());
}

void GameLayer::notifyPlayer(string info) {
    notice = Layer::create();
    auto label = Label::createWithTTF(info, "fonts/fangsong.ttf", 60);
    label->setPosition(Vec2(winMidX, winMidY - label->getContentSize().height));
    notice->addChild(label);
    this->getParent()->addChild(notice, 12);
}
void GameLayer::askPlayer(string info) {
    ask = Layer::create();

    auto label = Label::createWithTTF(info, "fonts/fangsong.ttf", 60);
    label->setPosition(Vec2(winMidX, winMidY - label->getContentSize().height));
    ask->addChild(label);

    auto yesBtn = cocos2d::ui::Button::create();
    yesBtn->setTouchEnabled(true);
    yesBtn->loadTextures("makesure.png", "makesure.png");
    yesBtn->setScale(tileScale/4);
    yesBtn->setPosition(Position(2, 6).toRealPos()-Vec2(tileSiz/2, 0));
    yesBtn->addTouchEventListener(CC_CALLBACK_2(GameLayer::yesBtnListener, this));
    auto noBtn = cocos2d::ui::Button::create();
    noBtn->setTouchEnabled(true);
    noBtn->loadTextures("cancel.png", "cancel.png");
    noBtn->setScale(tileScale/4);
    noBtn->setPosition(Position(3, 6).toRealPos()-Vec2(tileSiz/2, 0));
    noBtn->addTouchEventListener(CC_CALLBACK_2(GameLayer::noBtnListener, this));
    ask->addChild(yesBtn);
    ask->addChild(noBtn);

    this->getParent()->addChild(ask, 12);

}
void GameLayer::yesBtnListener(cocos2d::Ref *sender, cocos2d::ui::Widget::TouchEventType type) {
    PlayerSprite *player = playerSprites[turn];
    LandSprite *land = locateLand(player->p).getCurrent();
    
    if(tag == TAG_PURCHASE) {
        player->purchaseLand(land);
    }
    else if(tag == TAG_LEVELUP)
        player->levelupLand(land);
    
    noBtnListener(sender, type);
}
void GameLayer::noBtnListener(cocos2d::Ref *sender, cocos2d::ui::Widget::TouchEventType type) {
    Director::getInstance()->getEventDispatcher()->removeEventListenersForTarget(ask);
    ask->removeFromParent();
    ask = NULL;
    nextTurn();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene Change

void GameLayer::goShop() {
    Director::getInstance()->getEventDispatcher()->removeAllEventListeners();
    CCDirector::getInstance()->pushScene(ShopLayer::createScene(playerSprites[turn]->ticket));
}
void GameLayer::goPause() {
    Director::getInstance()->getEventDispatcher()->removeAllEventListeners();
    CCDirector::getInstance()->pushScene(PauseLayer::createScene());
}
void GameLayer::gameOver(int lastLoser) {
    int winner = pnum[++turn%2];
    Director::getInstance()->getEventDispatcher()->removeAllEventListeners();
    CCDirector::getInstance()->replaceScene(OverLayer::createScene(winner));
}
void GameLayer::shopCallBack(Ref *pSender) {
    initEventListener();
    PlayerSprite *player = playerSprites[turn];
    for(int i = 0; i < ITEM_KINDS; i++) {
        player->items[i] += add[i];
        player->ticket -= (add[i] * itemCost[i]);
    }
    updateToolsLayer();
}
void GameLayer::defaultCallBack(Ref *pSender) {
    initEventListener();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Touch Methods

// help create a moveable map
bool GameLayer::touchBegan(cocos2d::Touch *touch, cocos2d::Event *event){
    Point touchLoc = touch->getLocation();
    prvTouchLoc = touchLoc;
    
    if(isMoving) return true;
    if(notice != NULL) {
        notice->removeFromParent();
        notice = NULL;
        return true;
    }
    if(ask != NULL) {
        return true;
    }
//    if(tmpLabel != NULL) {
//        tmpLabel->removeFromParent();
//        tmpLabel = NULL;
//        return true;
//    }
    Rect pauseBtnRec = pauseBtn->getBoundingBox();
    if(pauseBtnRec.containsPoint(touchLoc)) {
        goPause();
        return true;
    }
    Rect diceBtnRec = diceBtn->getBoundingBox();
    if(diceBtnRec.containsPoint(touchLoc)) {
        // roll dice...
        move(rollDice());
        return true;
    }
    Rect avatarBtnRec = avatarBtn->getBoundingBox();
    if(avatarBtnRec.containsPoint(touchLoc)) {
        //brokeProcedure(pnum[turn]);
        // for debug sake
        changePOV(playerSprites[turn]->p);
//        Director::getInstance()->getEventDispatcher()->removeAllEventListeners();
//        CCDirector::getInstance()->replaceScene(OverLayer::createScene(3));
        return true;
    }
    if(isPlantingWhat != ITEM_KINDS) {
        
        DoubleDList<LandSprite *>::DDListIte<LandSprite *> ci = locateLand(playerSprites[turn]->p), iter = ci;
        LandSprite *tmp = iter.getCurrent();
        // ignoring when the list might be empty
        if(landSprites.getSize() > (PLANT_DIST*2+1)) {
            int i = 0;
            while(i != (PLANT_DIST+1)){
                Vec2 rp = this->convertToWorldSpaceAR(iter.getCurrent()->getPosition());
                if(Rect(rp.x, rp.y, tileSiz, tileSiz).containsPoint(touchLoc)) {
                    iter.getCurrent()->putObj(isPlantingWhat);
                    return true;
                }
                iter.moveBack();
                i++;
            }
            i = 0;
            iter = ci;
            iter.moveFront();
            while(i != PLANT_DIST){
                Vec2 rp = this->convertToWorldSpaceAR(iter.getCurrent()->getPosition());
                if(Rect(rp.x, rp.y, tileSiz, tileSiz).containsPoint(touchLoc)) {
                    iter.getCurrent()->putObj(isPlantingWhat);
                    return true;
                }
                iter.moveFront();
                i++;
            }
        }
        else {
            do{
                Vec2 rp = this->convertToWorldSpaceAR(iter.getCurrent()->getPosition());
                if(Rect(rp.x, rp.y, tileSiz, tileSiz).containsPoint(touchLoc)) {
                    iter.getCurrent()->putObj(isPlantingWhat);
                    return true;
                }
                iter.moveBack();
            } while(iter.getCurrent() != tmp);
        }
    }

//    Vec2 currentPos = this->getPosition();
//    for(int i = 0; i < pnum.size(); i++) {
//        for(int j = 0; j < playerSprites[i]->properties.size(); j++) {
//            LandSprite *land = playerSprites[i]->properties[j];
//            Rect r = land->getBoundingBox();
//            Vec2 rp = land->p.toRealPos();
//            if(Rect(rp.x, rp.y, tileSiz, tileSiz).containsPoint(touchLoc)) {
//                ostringstream oss;
//                oss << "worth: " << land->data;
//                tmpLabel = Label::createWithTTF(oss.str(), "fonts/Marker Felt.ttf", 40);
//                tmpLabel->setPosition(land->p.toRealPosAbove());
//                this->addChild(tmpLabel, 10);
//                return true;
//            }
//        }
//    }
    return true;
}
void GameLayer::touchMoved(cocos2d::Touch *touch, cocos2d::Event *event){
    Point touchLoc = touch->getLocation();
    Vec2 difference(touchLoc.x - prvTouchLoc.x, touchLoc.y - prvTouchLoc.y);
    Vec2 currentPos = this->getPosition();
    if(currentPos.x + difference.x > 0 || currentPos.x + difference.x < winSiz.width - mapWidth)
        difference.x = 0;
    if(currentPos.y + difference.y < 0 || currentPos.y + difference.y > mapHeight - winSiz.height)
        difference.y = 0;
    this->setPosition(currentPos + difference);
    prvTouchLoc = touchLoc;
}
void GameLayer::touchEnded(cocos2d::Touch *touch, cocos2d::Event *event){
//    Point touchLoc = touch->getLocation();
//    log("x: %f, y: %f", touchLoc.x, touchLoc.y);
}
