#include <SDL_scancode.h>
#include <fstream>
#include "DPI/cDPIGame.h"
#include "DPI/SpritesBatch.h"
#include "DPI/cDPIVector2D.h"
#include "DPI/json.hpp"

const float ASPECT_RATIO = 4.0 / 3.0;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

double max(double a, double b)
{
    if (a >= b) return a;
    else return b;
}

double min(double a, double b)
{
    if (a <= b) return a;
    else return b;
}


struct Entity
{
    int textureId;
    DPI::cVector2D size;
};

enum class ScrollType
{
    None,
    HorizontalScroll,
    VerticallScroll,
    FreeScroll,
};

struct Texture
{
    int textureId;
    int IW, IH;
    int IRW, IRH;
    int posX, posY;
//    DPI::cRect bounds;
//    DPI::cRect ratioHBounds;
};

struct Layer
{
    std::vector<Texture> textures;
};

struct Background
{
    ScrollType scrollType;
    std::vector<Layer> layers;
};


struct Position
{
    int posX;
    int posY;
};
enum class CameraLockType
{
    XLocked,
    YLocked,
};

class Camera
{
public:
    long int xPosition;
    long int yPosition;
private:
    int width;
    int height;
    CameraLockType lockType;
};


/****
 * ECS_
 */

struct World
{
    double unit_size;
    double width, height;
};

struct NTexture
{
    int textureId;
    DPI::cRect position;
};

struct NBackground
{
    std::vector<NTexture> textures;
    double virtual_width, virtual_height;
};

struct NCamera
{
    double x, y, w, h;
};


class MegamanXGame : public DPI::cGame, public DPI::cInputListener
{
public:
    MegamanXGame();
    ~MegamanXGame() override;
    void update(int deltaTime) override;
    void show() override;
    void onKeyUp(int key) override;
    void onKeyDown(int key) override;

private:
    bool mRightKeyPressed;
    bool mLeftKeyPressed;
    Position mCamera;
    Background mBackground;
    DPI::cTextureManager *mTextureManager;
    DPI::SpritesBatch mBatch;

    World world;
    std::vector<NBackground> backgrounds;
    NCamera camera;


    void moveCameraLeft(int deltaTime);
    void moveCameraRight(int deltaTime);
    Layer loadLayers();
    void drawBackGround();
    DPI::cTextureRegion getTextureRegionFromCamera(Position camera, Texture texture);
    DPI::cTextureRegion getLeftTextureRegionFromCamera(Position camera, Texture texture);
    DPI::cTextureRegion getRightTextureRegionFromCamera(Position camera, Texture texture);
};

//MegamanXGame::MegamanXGame() : cGame("./libs/", SCREEN_WIDTH, SCREEN_HEIGHT)
//{
//    this->getInput()->addListener(this);
//    mTextureManager = getGraphics()->mTextureManager;
//    mBackground.scrollType = ScrollType::HorizontalScroll;
//
//    mBackground.layers.push_back(loadLayers());
//    mTextureManager->setCurrentTexture(mBackground.layers[0].textures[0].textureId);
//
//    mCamera = Position{0, 0};
//    mRightKeyPressed = false;
//    mLeftKeyPressed = false;
//}

MegamanXGame::MegamanXGame() : cGame("./libs/", SCREEN_WIDTH, SCREEN_HEIGHT)
{
    // First lets configure our input listener
    this->getInput()->addListener(this);
    mTextureManager = getGraphics()->mTextureManager;

    // Lets load the level json file
    std::ifstream f("./assets/level1.json");
    std::string level1Json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    auto jsonObject = nlohmann::json::parse(level1Json);

    // Configure world
    world.unit_size = jsonObject["unit"];
    world.width = jsonObject["world"]["width"];
    world.height = jsonObject["world"]["height"];

    // Create and configure the backgrounds
    for (auto background: jsonObject["backgrounds"]) {
        NBackground bg;
        bg.virtual_width = background["width"];
        bg.virtual_height = background["height"];
        for (auto texture: background["textures"]) {
            NTexture t{};
            t.textureId = mTextureManager->getIdFromTexture(texture["file"]);
            t.position = DPI::cRect{
                    texture["rect"]["x"],
                    texture["rect"]["y"],
                    texture["rect"]["w"],
                    texture["rect"]["h"]
            };
            bg.textures.emplace_back(t);
        }
        backgrounds.emplace_back(bg);
    }

    // Configure Camera
    camera.x = 0.0;
    camera.y = 0.0;
    camera.w = jsonObject["camera"]["viewport"]["width"];
    camera.h = jsonObject["camera"]["viewport"]["height"];

    mRightKeyPressed = false;
    mLeftKeyPressed = false;
}


//MegamanXGame::~MegamanXGame()
//{
//    for (auto texture : mBackground.layers[0].textures) {
//        mTextureManager->releaseTexture(texture.textureId);
//    }
//}

MegamanXGame::~MegamanXGame()
{
    for (const auto &bg: backgrounds) {
        for (auto texture : bg.textures) {
            mTextureManager->releaseTexture(texture.textureId);
        }
    }
}

void MegamanXGame::update(int deltaTime)
{
    if (mRightKeyPressed) {
        moveCameraRight(deltaTime);
    } else if (mLeftKeyPressed) {
        moveCameraLeft(deltaTime);
    }
    cGame::update(deltaTime);
}

//void MegamanXGame::show()
//{
//    drawBackGround();
//    cGame::show();
//}

void MegamanXGame::show()
{

    // We need to calculate based on the camera position what part of the world we are showing.

    // Draw backgrounds
    for (const auto &background: backgrounds) {
        auto camera_virtual = DPI::cRect{
                static_cast<float>(camera.x * background.virtual_width / world.width),
                static_cast<float>(camera.y * background.virtual_height / world.height),
                static_cast<float>(camera.w * background.virtual_width / world.width),
                static_cast<float>(camera.h * background.virtual_height / world.height)
        };
        for (auto texture: background.textures) {
            bool cond1 = (camera_virtual.getX() >= texture.position.getX() &&
                          camera_virtual.getX() < texture.position.getX2());

            bool cond2 = (camera_virtual.getX2() >= texture.position.getX() &&
                          camera_virtual.getX2() < texture.position.getX2());
            cond1 = cond1;
            if (cond1 || cond2) {
                double block_x1 = max(camera_virtual.getX(), texture.position.getX());
                double block_x2 = min(camera_virtual.getX2(), texture.position.getX2());
                double tx1 = ((block_x1 - texture.position.getX()) / texture.position.getWidth());
                double tx2 = ((block_x2 - texture.position.getX()) / texture.position.getWidth());
                auto source = DPI::cTextureRegion{
                        static_cast<float>(tx1),
                        0.0,
                        static_cast<float>(tx2),
                        1.0,
                };

                double dest_x = (block_x1 - camera_virtual.getX()) / camera_virtual.getWidth();
                double dest_width = ((block_x2 - camera_virtual.getX()) / camera_virtual.getWidth()) - dest_x;
                auto dest = DPI::cRect{
                        SCREEN_WIDTH * static_cast<float>(dest_x),
                        0.0,
                        SCREEN_WIDTH * static_cast<float>(dest_width),
                        SCREEN_HEIGHT * 1.0
                };
                //printf("Texture %d: (%f -> %f)\n", texture.textureId, tx1, tx2);
                //printf("Dest: (%f -> %f)\n", dest.getX(), dest.getX2());
                mTextureManager->setCurrentTexture(texture.textureId);

                // TODO: Sprite batcher cannot draw from different textures in a single flush
                getGraphics()->mSpriteBatcher->drawSprite(dest, source);
                getGraphics()->mSpriteBatcher->flush();
            }
        }
    }
    //getGraphics()->mSpriteBatcher->flush();
    cGame::show();
}

void MegamanXGame::moveCameraLeft(int deltaTime)
{
    auto delta = (1.0 / 60.0) * deltaTime;
//    mCamera.posX -= static_cast<int>(20 * delta);
//    if (mCamera.posX < 0) {
//        mCamera.posX = 0;
//    }
    camera.x -= 2 * delta;
    if (camera.x < 0) {
        camera.x = 0;
    }
}

void MegamanXGame::moveCameraRight(int deltaTime)
{
    auto delta = (1.0 / 60.0) * deltaTime;
//    mCamera.posX += static_cast<int>(20 * delta);
//    auto limit = 513 + 513 + 3073;
//    if (mCamera.posX >= limit) {
//        mCamera.posX -= limit;
//    }

    camera.x += 2 * delta;
    if (camera.x > world.width - camera.w) {
        camera.x = 0;
    }
}


void MegamanXGame::drawBackGround()
{
    auto dst = DPI::cRect{
            0,
            0,
            800,
            600
    };

    Texture tex1 = mBackground.layers[0].textures[0];
    Texture tex2 = mBackground.layers[0].textures[1];
    Texture tex3 = mBackground.layers[0].textures[2];
    int CX2 = static_cast<int>(mCamera.posX) + SCREEN_WIDTH;

    if (CX2 <= tex1.IRW) {
        auto src = getTextureRegionFromCamera(mCamera, tex1);
        getGraphics()->mTextureManager->setCurrentTexture(tex1.textureId);
        mBatch.addSprite(dst, src);
        getGraphics()->mSpriteBatcher->drawSprite(dst, src);
    } else if (mCamera.posX <= tex1.IRW && CX2 > tex2.posX) {
        auto cut = static_cast<float>(tex1.IRW - mCamera.posX);
        auto dst_p1 = DPI::cRect{
                0,
                0,
                cut,
                SCREEN_HEIGHT,
        };
        auto dst_p2 = DPI::cRect{
                cut,
                0,
                SCREEN_WIDTH - cut,
                SCREEN_HEIGHT,
        };
        auto src_1 = getLeftTextureRegionFromCamera(mCamera, tex1);
        auto src_2 = getRightTextureRegionFromCamera(mCamera, tex2);

        getGraphics()->mTextureManager->setCurrentTexture(tex1.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst_p1, src_1);
        getGraphics()->mTextureManager->setCurrentTexture(tex2.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst_p2, src_2);

    } else if (mCamera.posX >= tex2.posX && CX2 < tex2.posX + tex2.IRW) {
        float x1 = static_cast<float>(mCamera.posX - tex2.posX) / static_cast<float>(tex2.IRW);
        float x2 = static_cast<float>(CX2 - tex2.posX) / static_cast<float>(tex2.IRW);
        auto src = DPI::cTextureRegion{
                x1,
                0.0,
                x2,
                1.0
        };
        getGraphics()->mTextureManager->setCurrentTexture(tex2.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst, src);
    } else if (mCamera.posX < tex2.posX + tex2.IRW && CX2 >= tex3.posX) {
        auto cut = static_cast<float>(tex2.posX + tex2.IRW - mCamera.posX);
        auto dst_p1 = DPI::cRect{0, 0, cut, SCREEN_HEIGHT,};
        auto dst_p2 = DPI::cRect{cut, 0, SCREEN_WIDTH - cut, SCREEN_HEIGHT,};

        auto src_1 = getLeftTextureRegionFromCamera(mCamera, tex2);
        auto src_2 = getRightTextureRegionFromCamera(mCamera, tex3);

        getGraphics()->mTextureManager->setCurrentTexture(tex2.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst_p1, src_1);
        getGraphics()->mSpriteBatcher->flush();
        getGraphics()->mTextureManager->setCurrentTexture(tex3.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst_p2, src_2);
    } else if (mCamera.posX >= tex3.posX) {
        auto src = getTextureRegionFromCamera(mCamera, tex3);
        getGraphics()->mTextureManager->setCurrentTexture(tex3.textureId);
        getGraphics()->mSpriteBatcher->drawSprite(dst, src);
    }

    getGraphics()->mSpriteBatcher->flush();
}

DPI::cTextureRegion MegamanXGame::getTextureRegionFromCamera(Position camera, Texture texture)
{
    int CX2 = camera.posX + SCREEN_WIDTH;
    float x1 = static_cast<float>(camera.posX - texture.posX) / static_cast<float>(texture.IRW);
    float x2 = static_cast<float>(CX2 - texture.posX) / static_cast<float>(texture.IRW);
    return DPI::cTextureRegion{
            x1,
            0.0,
            x2,
            1.0
    };
}

DPI::cTextureRegion MegamanXGame::getLeftTextureRegionFromCamera(Position camera, Texture texture)
{
    float xa1 = static_cast<float>(camera.posX - texture.posX) / static_cast<float>(texture.IRW);
    float xa2 = 1.0f;

    auto p1 = DPI::cTextureRegion{
            xa1,
            0.0,
            xa2,
            1.0
    };

    return p1;
}

DPI::cTextureRegion MegamanXGame::getRightTextureRegionFromCamera(Position camera, Texture texture)
{
    int CX2 = camera.posX + SCREEN_WIDTH;
    float xb1 = 0.0f;
    float xb2 = static_cast<float>(CX2 - texture.posX) / static_cast<float>(texture.IRW);

    auto p2 = DPI::cTextureRegion{
            xb1,
            0.0,
            xb2,
            1.0,
    };

    return p2;
}


void MegamanXGame::onKeyUp(int key)
{
    switch (key) {
        case SDL_SCANCODE_ESCAPE:
            stop();
            break;
        case SDL_SCANCODE_RIGHT:
            mRightKeyPressed = false;
            break;
        case SDL_SCANCODE_LEFT:
            mLeftKeyPressed = false;
            break;
        default:
            break;
    }
}

void MegamanXGame::onKeyDown(int key)
{
    switch (key) {
        case SDL_SCANCODE_RIGHT:
            mRightKeyPressed = true;
            break;
        case SDL_SCANCODE_LEFT:
            mLeftKeyPressed = true;
            break;
        default:
            break;
    }
}


Layer MegamanXGame::loadLayers()
{
    std::string textureFiles[] = {"./assets/bg_1.png", "./assets/bg_1.png", "./assets/bg_2.png"};
    Layer layer;
    layer.textures.push_back(Texture{
            mTextureManager->getIdFromTexture(textureFiles[0]),
            513, 257,
            513 * SCREEN_HEIGHT / 257, SCREEN_HEIGHT,
            0, 0,
    });

    layer.textures.push_back(Texture{
            mTextureManager->getIdFromTexture(textureFiles[1]),
            513, 257,
            513 * SCREEN_HEIGHT / 257, SCREEN_HEIGHT,
            513 * SCREEN_HEIGHT / 257, 0
    });

    layer.textures.push_back(Texture{
            mTextureManager->getIdFromTexture(textureFiles[2]),
            3073, 256,
            3073 * SCREEN_HEIGHT / 256, SCREEN_HEIGHT,
            2 * 513 * SCREEN_HEIGHT / 257, 0
    });
    return layer;
}


int main(int argc, char *argv[])
{
    auto game = std::make_unique<MegamanXGame>();
    game->run();
    return 0;
}