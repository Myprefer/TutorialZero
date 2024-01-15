#include <iostream>
#include <graphics.h>
#include <conio.h>
#include <string>
#include <vector>
#include <math.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BUTTON_WIDTH 192
#define BUTTON_HEIGHT 75

#pragma comment(lib, "MSIMG32.LIB")
#pragma comment(lib, "Winmm.lib")

int world_level = 1;
bool isGameStart = false;
bool running = true;

inline void putimageAlpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img),
		0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

class Altas {
public:
	Altas(LPCTSTR path, int num) {
		TCHAR pathFile[256];
		for (size_t i = 0; i < num; i++) {
			_stprintf_s(pathFile, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, pathFile);
			frameList.push_back(frame);
		}
	}

	~Altas() {
		for (size_t i = 0; i < frameList.size(); i++) delete frameList[i];
	}

public:
	std::vector<IMAGE*> frameList;
};

Altas* altasPlayerLeft;
Altas* altasPlayerRight;
Altas* altasEnemyLeft;
Altas* altasEnemyRight;

class Animation {
public:
	Animation(Altas* altas, int interval) {
		intervalMs = interval;
		animAltas = altas;
	}

	~Animation() = default;

	void play(int x, int y, int delta) {
		timer += delta;
		if (timer >= intervalMs) {
			idxFrame = (idxFrame + 1) % animAltas->frameList.size();
			timer = 0;
		}

		putimageAlpha(x, y, animAltas->frameList[idxFrame]);
	}

private:
	int timer = 0;		//动画计时器
	int idxFrame = 0;	//动画帧索引
	int intervalMs = 0;

private:
	Altas* animAltas;
};

class Player {
public:
	Player() {
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(altasPlayerLeft, 45);
		anim_right = new Animation(altasPlayerRight, 45);
	}

	~Player() {
		delete anim_left;
		delete anim_right;
	}

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_KEYDOWN:
			switch (msg.vkcode) {
			case VK_UP:
				isMoveUp = true;
				break;
			case VK_DOWN:
				isMoveDown = true;
				break;
			case VK_LEFT:
				isMoveLeft = true;
				break;
			case VK_RIGHT:
				isMoveRight = true;
				break;
			}
			break;

		case WM_KEYUP:
			switch (msg.vkcode) {
			case VK_UP:
				isMoveUp = false;
				break;
			case VK_DOWN:
				isMoveDown = false;
				break;
			case VK_LEFT:
				isMoveLeft = false;
				break;
			case VK_RIGHT:
				isMoveRight = false;
				break;
			}
			break;
		}
	}

	void Move() {
		int dirX = isMoveRight - isMoveLeft;
		int dirY = isMoveDown - isMoveUp;
		double lenDir = sqrt(dirX * dirX + dirY * dirY);

		if (lenDir) {
			double normalizedX = dirX / lenDir;
			double normalizedY = dirY / lenDir;
			position.x += (int)(SPEED * normalizedX);
			position.y += (int)(SPEED * normalizedY);
		}

		if (position.x < 0) position.x = 0;
		if (position.y < 0) position.y = 0;
		if (position.x + FRAME_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - FRAME_WIDTH;
		if (position.y + FRAME_HEIGHT > WINDOW_HEIGHT) position .y = WINDOW_HEIGHT - FRAME_HEIGHT;
	}

	void Draw(int delta) {
		int posShadowX = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int posShadowY = position.y + FRAME_HEIGHT - 8;
		putimageAlpha(posShadowX, posShadowY, &img_shadow);

		static bool facingLeft = false;
		int dirX = isMoveRight - isMoveLeft;
		if (dirX < 0) facingLeft = true;
		else if (dirX > 0) facingLeft = false;
		
		if (facingLeft) anim_left->play(position.x, position.y, delta);
		else anim_right->play(position.x, position.y, delta);
	}

	const POINT& GetPosition() const {
		return position;
	}

	const int GetFrameWidth() const {
		return FRAME_WIDTH;
	}

	const int GetFrameHeight() const {
		return FRAME_HEIGHT;
	}

private:
	const int SPEED = 3;
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;
	const int SHADOW_WIDTH = 32;

private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 500, 500 };
	bool isMoveUp = false;
	bool isMoveDown = false;
	bool isMoveLeft = false;
	bool isMoveRight = false;
};

class Bullet {
public:
	POINT position = { 0 ,0 };

public:
	Bullet() = default;
	~Bullet() = default;

	void Draw() const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10;
};

class Enemy {
public:
	Enemy() {
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(altasEnemyLeft, 45);
		anim_right = new Animation(altasEnemyRight, 45);

		//敌人生成的边界
		enum class SpawnEdge {
			Up = 0,
			Down,
			Left,
			Right
		};

		//将敌人放置在地图外边界处的随机位置
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::Up:
			position.x = rand() % WINDOW_WIDTH;
			position.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down:
			position.x = rand() % WINDOW_WIDTH;
			position.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			position.x = -FRAME_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			position.x = WINDOW_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}
		bool CheckBulletCollision(const Bullet& bullet) {
			//将子弹等效为点, 判断点是否在敌人矩形内
			bool isOverLapX = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
			bool isOverLapY = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
			return isOverLapX && isOverLapY;
		}

		bool CheckPlayerCollision(const Player& player) {
			//敌人的中心点作为判定点
			POINT checkPosition = { position.x + FRAME_WIDTH / 2, position.y + FRAME_HEIGHT / 2 };
			
			bool isOverLapX = checkPosition.x >= player.GetPosition().x && checkPosition.x <= player.GetPosition().x + player.GetFrameWidth();
			bool isOverLapY = checkPosition.y >= player.GetPosition().y && checkPosition.y <= player.GetPosition().y + player.GetFrameHeight();
			return isOverLapX && isOverLapY;
		}

		void Move(const Player& player) {
			const POINT& playerPositon = player.GetPosition();
			int dirX = playerPositon.x - position.x;
			int dirY = playerPositon.y - position.y;
			double lenDir = sqrt(dirX * dirX + dirY * dirY);
			if (lenDir) {
				double normalizedX = dirX / lenDir;
				double normalizedY = dirY / lenDir;
				position.x += (int)(SPEED * normalizedX);
				position.y += (int)(SPEED * normalizedY);
			}

			if (dirX < 0) facingLeft = true;
			else if (dirX > 0) facingLeft = false;
		}

		void Draw(int delta) {
			int posShadowX = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
			int posShadowY = position.y + FRAME_HEIGHT - 35;
			putimageAlpha(posShadowX, posShadowY, &img_shadow);

			if (facingLeft) anim_left->play(position.x, position.y, delta);
			else anim_right->play(position.x, position.y, delta);
		}

		void Hurt() {
			alive = false;
		}

		bool CheckAlive() {
			return alive;
		}

	~Enemy() {
		delete anim_left;
		delete anim_right;
	}

private:
	const int SPEED = 2 * (1 + 0.2 * world_level);
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;
	const int SHADOW_WIDTH = 48;

private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position{ 0 ,0 };
	bool facingLeft = false;
	bool alive = true;
};

class Button {
public:
	Button(RECT rect, LPCTSTR pathImgIdle, LPCTSTR pathImgHovered, LPCTSTR pathImgPushed) {
		region = rect;

		loadimage(&img_idle, pathImgIdle);
		loadimage(&img_hovered, pathImgHovered);
		loadimage(&img_pushed, pathImgPushed);
	}

	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y)) status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed) OnClick();
			break;
		default:
			break;
		}
	}

	~Button() = default;

protected:
	virtual void OnClick() = 0;

private:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed,
	};

private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;

private:
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y <= region.bottom && y >= region.top;
	}
};

class StartGameButton : public Button {
public:
	StartGameButton(RECT rect, LPCTSTR pathImgIdle, LPCTSTR pathImgHovered, LPCTSTR pathImgPushed)
		: Button(rect, pathImgIdle, pathImgHovered, pathImgPushed) {}

	~StartGameButton() = default;

protected:
	void OnClick() {
		isGameStart = true;

		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

class QuitGameButton : public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR pathImgIdle, LPCTSTR pathImgHovered, LPCTSTR pathImgPushed)
		: Button(rect, pathImgIdle, pathImgHovered, pathImgPushed) {}

	~QuitGameButton() = default;

protected:
	void OnClick() {
		running = false;
	}
};

void TryGenerateEnemy(std::vector<Enemy*>& enemyList) {
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0)  enemyList.push_back(new Enemy());
}

void UpdateBullets(std::vector<Bullet>& bulletList, const Player& player) {
	const double RADIAL_SPEED = 0.0045; //径向波动速度
	const double TANGENT_SPEED = 0.0025 * world_level; //切向波动速度
	double radianInterval = 2 * 3.14159 / bulletList.size(); //子弹间隔
	POINT playerPosition = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
	for (size_t i = 0; i < bulletList.size(); i++) {
		double radian = GetTickCount() * TANGENT_SPEED + radianInterval * i;
		bulletList[i].position.x = playerPosition.x + player.GetFrameWidth() / 2 + (int)(radius * sin(radian));
		bulletList[i].position.y = playerPosition.y + player.GetFrameHeight() / 2 + (int)(radius * cos(radian));
	}
}

void DrawPlayerScore(int score, int level) {
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分: %d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);

	static TCHAR levelText[64];
	_stprintf_s(text, _T("世界等级: %d 级"), level);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 30, text);
}

int main() {
	initgraph(1280, 720);

	altasPlayerLeft = new Altas(_T("img/player_left_%d.png"), 6);
	altasPlayerRight = new Altas(_T("img/player_right_%d.png"), 6);
	altasEnemyLeft = new Altas(_T("img/enemy_left_%d.png"), 6);
	altasEnemyRight = new Altas(_T("img/enemy_right_%d.png"), 6);

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	int score = 0;

	ExMessage msg;
	Player player;
	IMAGE img_menu;
	IMAGE img_backgraound;
	std::vector<Enemy*> enemyList;
	std::vector<Bullet> bulletList(2);

	RECT regionButtonStartGame;
	RECT regionButtonQuitGame;

	regionButtonStartGame.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	regionButtonStartGame.right = regionButtonStartGame.left + BUTTON_WIDTH;
	regionButtonStartGame.top = 430;
	regionButtonStartGame.bottom = regionButtonStartGame.top + BUTTON_HEIGHT;

	regionButtonQuitGame.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	regionButtonQuitGame.right = regionButtonQuitGame.left + BUTTON_WIDTH;
	regionButtonQuitGame.top = 550;
	regionButtonQuitGame.bottom = regionButtonQuitGame.top + BUTTON_HEIGHT;

	StartGameButton buttonStartGame = StartGameButton(regionButtonStartGame,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton buttonQuitGame = QuitGameButton(regionButtonQuitGame,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_backgraound, _T("img/background.png"));

	BeginBatchDraw();

	while (running) {
		DWORD startTime = GetTickCount();

		while (peekmessage(&msg)) {
			if (isGameStart) player.ProcessEvent(msg);
			else {
				buttonStartGame.ProcessEvent(msg);
				buttonQuitGame.ProcessEvent(msg);
			}
		}

		if (isGameStart) {
			//玩家移动
			player.Move();

			//生成敌人
			TryGenerateEnemy(enemyList);

			//敌人移动
			for (Enemy* enemy : enemyList) enemy->Move(player);

			//更新子弹
			UpdateBullets(bulletList, player);

			//检测敌人与玩家的碰撞
			for (Enemy* enemy : enemyList) {
				if (enemy->CheckPlayerCollision(player)) {
					mciSendString(_T("close bgm"), NULL, 0, NULL);
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分: %d !"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}

			//检测子弹碰撞敌人
			for (Enemy* enemy : enemyList) {
				for (const Bullet& bullet : bulletList) {
					if (enemy->CheckBulletCollision(bullet)) {
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->Hurt();
						score++;
						if (score % 10 == 0) {
							world_level++;
						}
					}
				}
			}

			//移除敌人
			for (size_t i = 0; i < enemyList.size(); i++) {
				Enemy* enemy = enemyList[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemyList[i], enemyList.back());
					enemyList.pop_back();
					delete enemy;
				}
			}
		}

		//画面渲染
		cleardevice();
		
		if (isGameStart) {
			putimage(0, 0, &img_backgraound);
			player.Draw(1000 / 144);
			for (Enemy* enemy : enemyList) enemy->Draw(1000 / 144);
			for (const Bullet& bullet : bulletList) bullet.Draw();
			DrawPlayerScore(score, world_level);
		}
		else {
			putimage(0, 0, &img_menu);
			buttonStartGame.Draw();
			buttonQuitGame.Draw();
		}
	
		FlushBatchDraw();

		DWORD endTime = GetTickCount();
		DWORD deltaTime = endTime - startTime;
		if (deltaTime < 1000 / 144) Sleep(1000 / 144 - deltaTime);
	}

	delete altasPlayerLeft;
	delete altasPlayerRight;
	delete altasEnemyLeft;
	delete altasEnemyRight;

	EndBatchDraw();

	return 0;
}