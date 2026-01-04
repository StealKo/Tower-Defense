#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

typedef struct{
    int x;
    int y;
} Path;

typedef struct{
    float x;
    float y;
} Position;

typedef struct{
    char name[50];
    Position position;
    int waypoint;
    int active;
    int damage;
    int hp;
    float speed;
    float base_speed;
    Uint32 slow_timer;
    int reward;
    int score;
    SDL_FRect rect;
    SDL_FRect frame;
    SDL_Texture* texture;
} Monster;

typedef struct{
    char name[50];
    int price;
    int damage;
    int level;
    int range;
    float cooldown;
    float cooldown_timer;
    int shot_fired;
    Uint32 start_time;
    int slow_effect;
    int slow_multiplier;
    SDL_FRect rect;
    SDL_FRect frame;
    SDL_Texture* texture;
} Tower;

typedef struct{
    SDL_FRect rect;
    int active;
    int tower_index;
} TowerSlot;

typedef struct{
    int zombies;
    int slimes;
} Wave;

#define MAX_TOWERS 5
TowerSlot tower_slot[MAX_TOWERS] = {
    {{106, 90, 60, 60}, 0, -1},
    {{160, 240, 60, 60}, 0, -1},
    {{290, 330, 60, 60}, 0, -1},
    {{450, 330, 60, 60}, 0, -1},
    {{585, 180, 60, 60}, 0, -1}
};
Tower built_towers[MAX_TOWERS];
int built_towers_count = 0;
int selected_slot_index = -1;

#define MAX_MONSTERS 50
Monster active_monsters[MAX_MONSTERS];
int active_monsters_count = 0;

#define MAX_WAVES 10
Wave waves[MAX_WAVES] = {
    {6, 0},
    {8, 2},
    {10, 4},
    {0, 10},
    {10, 10},
    {10, 15},
    {20, 20}
};
int current_wave = 0;
Uint32 wave_start_time = 0;
int waiting_for_wave = 1;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* map_texture;
SDL_Texture* monster;
SDL_Texture* tower;
SDL_Texture* hp_texture;
SDL_Texture* coins_texture;
SDL_Texture* score_texture;
SDL_Texture* option_texture[4];
TTF_Font* font;

Monster monsters[] = {
    {"Zombie", {0, 173}, 0, 1, 5, 50, 0.05, 0.05, 0, 5, 10, {10, 16, 10, 16}, {0, 0, 10, 16}, NULL},
    {"Slime", {0, 173}, 0, 1, 10, 70, 0.05, 0.05, 0, 10, 15, {10, 16, 10, 16}, {15, 0, 10, 16}, NULL}
};

Tower towers[] = {
    {"Defalut Tower", 10, 10, 1, 175, 1000, 0, 0, 0, 0, 0, {0, 0, 120, 120}, {0, 0, 43, 41}, NULL},
    {"Slow Tower", 15, 5, 1, 175, 2000, 0, 0, 0, 1, 2, {0, 0, 120, 120}, {0, 84, 43, 41}, NULL},
    {"Fast Tower", 15, 7, 1, 175, 300, 0, 0, 0, 0, 0, {0, 0, 120, 120}, {0, 42, 43, 41}, NULL}
};

/* waypointy */
Path path_waypoints[] = {  
    {0, 173},
    {243, 173},    
    {243, 410}, 
    {535, 410}, 
    {535, 115}, 
    {800,115}
};

int coins = 25;
int coins_width = 0;
int coins_height = 0;
int last_coins = -1;

int health = 100;
int health_width = 0;
int health_height = 0;
int last_health = -1;

int score = 0;
int score_width = 0;
int score_height = 0;
int last_score = -1;

Uint32 next_spawn_time = 0;
int total_monsters = 0;
int spawned = 0;

int tower_menu_active = 0;
int tower_type = -1;

/* tlačítka pro věže */
SDL_FRect tower_option1 = {320, 200, 200, 60};
SDL_FRect tower_option2 = {320, 240, 200, 60};
SDL_FRect tower_option3 = {320, 280, 200, 60};
SDL_FRect tower_option4 = {320, 320, 200, 60};

/* funkce pro pohyb monster po cestě */
void UpdateMonster(Monster* monster){
    if(monster -> waypoint >= 6){
        if(monster -> active){
            health -= monster -> damage;
            if(health < 0){
                health = 0;
            }
        }
        monster -> active = 0;
        return;
    }

    Path target = path_waypoints[monster -> waypoint];

    if(monster -> position.x < target.x){
        monster -> position.x += monster -> speed;

        if(monster -> position.x > target.x){
            monster -> position.x = target.x;
        }
    }
    else if(monster -> position.x > target.x){
        monster -> position.x -= monster -> speed;

        if(monster -> position.x < target.x){
            monster -> position.x = target.x;
        }
    }

    if(monster -> position.y < target.y){
        monster -> position.y += monster -> speed;

        if(monster -> position.y > target.y){
            monster -> position.y = target.y;
        }
    }
    else if(monster -> position.y > target.y){
        monster -> position.y -= monster -> speed;

        if(monster -> position.y < target.y){
            monster -> position.y = target.y;
        }
    }

    if(monster -> slow_timer > 0){
        monster -> slow_timer -= 10;
        if(monster -> slow_timer <= 0){
            monster -> speed = monster -> base_speed;
        }
    }

    if(monster -> position.x == target.x && monster -> position.y == target.y){
        monster -> waypoint++;
    }
}

void UpdateWave(){
    if(current_wave >= MAX_WAVES){
        return;
    }

    Wave wave = waves[current_wave];
    Uint32 now = SDL_GetTicks();

    if(wave_start_time == 0){
        wave_start_time = now;
        waiting_for_wave = 1;
    }

    if(waiting_for_wave){
        if(now - wave_start_time < 5000){
            return;
        }
        waiting_for_wave = 0;
    }

    /* vytvoření vlny */
    if(total_monsters == 0){
        total_monsters = wave.zombies + wave.slimes;
        spawned = 0;
        next_spawn_time = now + 1000;
    }

    if(spawned < total_monsters && now >= next_spawn_time){
        int type;

        if(spawned < wave.zombies){
            /* zombie */
            type = 0;
        }
        else{
            /* slime */
            type = 1;
        }

        if(active_monsters_count < MAX_MONSTERS){
            Monster new_monster = monsters[type];

            if(current_wave > 1){
                new_monster.hp *= 1.5;
                new_monster.damage *= 1.5;
                new_monster.reward *= 1.5; 
            }

            new_monster.active = 1;
            new_monster.waypoint = 0;
            new_monster.position.x = (float)path_waypoints[0].x;
            new_monster.position.y = (float)path_waypoints[0].y;
            new_monster.speed = new_monster.base_speed;
            new_monster.slow_timer = 0;

            active_monsters[active_monsters_count++] = new_monster;
        }
        spawned++;
        next_spawn_time = now + 1000;
    }

    int remaining_monsters = 0;
    for(int i = 0; i < active_monsters_count; i++){
        if(active_monsters[i].active){
            remaining_monsters++;
        }
    }

    if(remaining_monsters == 0 && spawned == total_monsters){
        current_wave++;
        total_monsters = 0;
        spawned = 0;
        active_monsters_count = 0;
        waiting_for_wave = 1;
        wave_start_time = SDL_GetTicks();
    }
}

Monster* MonsterInTowerRange(Tower* tower){
    Monster* target = NULL;
    float range = tower -> range * tower -> range;

    /* střed věže */
    float tower_x = tower -> rect.x + tower -> rect.w / 2;
    float tower_y = tower -> rect.y + tower -> rect.h / 2;

    for(int i = 0; i < active_monsters_count; i++){
        Monster* monster = &active_monsters[i];

        if(!monster -> active){
            continue;
        }

        /* vzdálenost monstra od věže */
        float distance_x = monster -> position.x - tower_x;
        float distance_y = monster -> position.y - tower_y;
        float distance = distance_x * distance_x + distance_y * distance_y;

        if(distance < range){
            range = distance;
            target = monster;
        }
    }

    return target;
}

/* funkce pro update životů */
void UpdatePlayerHealth(SDL_Renderer* renderer, TTF_Font* font, int health){
    if(health == last_health){
        return;
    }

    /* zničení staré textury */
    if(hp_texture){
        SDL_DestroyTexture(hp_texture);
        hp_texture = NULL;
    }
    
    /* Text pro životy */
    char health_text[50];
    sprintf(health_text, "Životy: %d", health);
    SDL_Color color = {255, 255, 255, 255};
    
    /* vytvoření surface a textury */
    SDL_Surface* surface = TTF_RenderText_Solid(font, health_text, strlen(health_text), color);
    hp_texture = SDL_CreateTextureFromSurface(renderer, surface);

    health_width = surface -> w;
    health_height = surface -> h;

    SDL_DestroySurface(surface);

    /* aktualizace životů*/
    last_health = health;
}

void UpdatePlayerCoins(SDL_Renderer* renderer, TTF_Font* font, int coins){
    if(coins == last_coins){
        return;
    }

    /* zničení staré textury */
    if(coins_texture){
        SDL_DestroyTexture(coins_texture);
        coins_texture = NULL;
    }

    /* vytvoření textu */
    char coins_text[50];
    sprintf(coins_text, "Peníze: %d", coins);
    SDL_Color color = {255, 255, 255, 255};

    /* vytvoření surface a textury */
    SDL_Surface* surface = TTF_RenderText_Solid(font, coins_text, strlen(coins_text), color);
    coins_texture = SDL_CreateTextureFromSurface(renderer, surface);

    coins_width = surface -> w;
    coins_height = surface -> h;

    SDL_DestroySurface(surface);

    /* aktualizace peněz */
    last_coins = coins;
}

void UpdatePlayerScore(SDL_Renderer* renderer, TTF_Font* font, int score){
    if(score == last_score){
        return;
    }

    /* zničení staré textury */
    if(score_texture){
        SDL_DestroyTexture(score_texture);
        score_texture = NULL;
    }

    /* vytvoření textu */
    char score_text[50];
    sprintf(score_text, "Skóre: %d", score);
    SDL_Color color = {255, 255, 255, 255};

    SDL_Surface* surface = TTF_RenderText_Solid(font, score_text, strlen(score_text), color);
    score_texture = SDL_CreateTextureFromSurface(renderer, surface);

    score_width = surface -> w;
    score_height = surface -> h;

    SDL_DestroySurface(surface);

    /* aktualizace skóre */
    last_score = score;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv){
    if(!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("Chyba při načtení SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* vytvoření okna */
    window = SDL_CreateWindow(
        "Tower Defense",
        800,
        600,
        SDL_WINDOW_BORDERLESS
    );

    if(!window){
        SDL_Log("Chyba vytvoření okna: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* vytvoření rendereru */
    renderer = SDL_CreateRenderer(window, NULL);

    if(!renderer){
        SDL_Log("Chyba vytvoření rendereru: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* načtení mapy jako png obrázek */
    char map_path[] = "../assets/map.png";
    map_texture = IMG_LoadTexture(renderer, map_path);

    if(!map_texture){
        SDL_Log("Chyba načtení mapy: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* načtení monster */
    char monster_path[] = "../assets/characters.png";
    monster = IMG_LoadTexture(renderer, monster_path);

    if(!monster){
        SDL_Log("Chyba načtení monstra: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    for(int i = 0; i < 2; i++){
        monsters[i].texture = monster;
    }

    char tower_path[] = "../assets/towers.png";
    tower = IMG_LoadTexture(renderer, tower_path);

    for(int i = 0; i < 3; i++){
        towers[i].texture = tower;
    }

    if(!TTF_Init()){
        SDL_Log("Chyba načtení SDL_ttf");
        return SDL_APP_FAILURE;
    }

    font = TTF_OpenFont("../assets/ARIALNB.TTF", 24);
    if(!font){
        SDL_Log("Chyba načtení fontu");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    if(event -> type == SDL_EVENT_KEY_DOWN){
        if(event -> key.key == SDLK_ESCAPE){
            return SDL_APP_SUCCESS;
        }
    }

    if(event -> type == SDL_EVENT_MOUSE_BUTTON_DOWN){
        float mouse_x = event->button.x;
        float mouse_y = event->button.y;

        SDL_FPoint mouse_position = {
            mouse_x,
            mouse_y
        };

        if(tower_menu_active){
            if(tower_type == -2){
                if(SDL_PointInRectFloat(&mouse_position, &tower_option1)){
                    Tower* tower = &built_towers[tower_slot[selected_slot_index].tower_index];

                    if(tower -> level < 3){
                        int upgrade_cost = tower -> price * 2;
                            
                        if(coins >= upgrade_cost){
                            coins -= upgrade_cost;
                            tower -> damage *= 2;
                            tower -> price = upgrade_cost;
                            tower -> level++;
                            tower -> slow_multiplier *= 2; 

                            UpdatePlayerCoins(renderer, font, coins);
                            SDL_Log("Věž upgradována - Nový damage: %d", tower -> damage);
                        }
                        else{
                            SDL_Log("Nemáte dostatek peněz na upgrade");
                        }
                        tower_menu_active = 0;
                    }
                    else{
                        SDL_Log("Věž je na maximálním levelu");
                    }    
                }
                else if(SDL_PointInRectFloat(&mouse_position, &tower_option2)){
                    tower_menu_active = 0;
                }       
            }
            else{
                if(SDL_PointInRectFloat(&mouse_position, &tower_option1)){
                    tower_type = 0;
                }
                else if(SDL_PointInRectFloat(&mouse_position, &tower_option2)){
                    tower_type = 1;
                }
                else if(SDL_PointInRectFloat(&mouse_position, &tower_option3)){
                    tower_type = 2;
                }
                else if(SDL_PointInRectFloat(&mouse_position, &tower_option4)){
                    tower_menu_active = 0;
                }
            }       
        }
        else{
            for(int i = 0; i < MAX_TOWERS; i++){
                if(SDL_PointInRectFloat(&mouse_position, &tower_slot[i].rect)){
                    selected_slot_index = i;
                    tower_menu_active = 1;
                    
                    if(tower_slot[i].active){
                        tower_type = -2;
                    }
                    else{
                        tower_type = -1;
                    }

                    SDL_Log("Klikl si na věž %d", i+1);
                    break;
                }
            }
        }
    
        /* umístění vybrané věže */
        if(tower_type >= 0 && selected_slot_index >= 0){
            Tower new_tower = towers[tower_type];
            TowerSlot* slot = &tower_slot[selected_slot_index];

            if(slot && !slot -> active && coins >= new_tower.price && built_towers_count < MAX_TOWERS){

                /* pozicování veže na políčko */
                new_tower.rect.x = slot -> rect.x - 5;
                new_tower.rect.y = slot -> rect.y - 55;

                built_towers[built_towers_count] = new_tower;
                slot -> tower_index = built_towers_count;
                built_towers_count++;

                coins -= new_tower.price;
                UpdatePlayerCoins(renderer, font, coins);

                slot -> active = 1;
            }
            
            tower_menu_active = 0;
            tower_type = -1;
        }  
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* vykreslení mapy */
    SDL_RenderTexture(renderer, map_texture, NULL, NULL);

    UpdateWave();

    /* update a render monster */
    for(int i = 0; i < active_monsters_count; i++){
        if(active_monsters[i].active){
            UpdateMonster(&active_monsters[i]);

            SDL_FRect monster_rect = {
                active_monsters[i].position.x,
                active_monsters[i].position.y,
                active_monsters[i].rect.w * 2,
                active_monsters[i].rect.h * 2
            };

            SDL_SetTextureScaleMode(active_monsters[i].texture, SDL_SCALEMODE_NEAREST);
            SDL_RenderTexture(renderer, active_monsters[i].texture, &active_monsters[i].frame, &monster_rect);
        }
    }

    Uint32 now = SDL_GetTicks();

    for(int i = 0; i < built_towers_count; i++){
        Tower* tower = &built_towers[i];

        SDL_SetTextureScaleMode(built_towers[i].texture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, built_towers[i].texture, &built_towers[i].frame, &built_towers[i].rect);

        if(tower -> cooldown_timer > 0){
            tower -> cooldown_timer -= 16;

            if(tower -> cooldown_timer < 0){
                tower -> cooldown_timer = 0;
            }
        }

        Monster* target = MonsterInTowerRange(&built_towers[i]);

        if(target != NULL){
            /* střed věže */
            float tower_x = built_towers[i].rect.x + built_towers[i].rect.w / 3;
            float tower_y = built_towers[i].rect.y + built_towers[i].rect.h / 2;

            /* střed monstra */
            float monster_x = target -> position.x + target -> rect.w / 2;
            float monster_y = target -> position.y + target -> rect.h / 2;

            float x1 = tower_x;
            float y1 = tower_y;
            float x2 = monster_x;
            float y2 = monster_y;

            if(tower -> shot_fired){
                if(now - tower -> start_time <= 500){
                    int steps = 100;
                    for(int j = 0; j <= steps; j++){
                        float t = (float)j / steps;
                        float x = x1 + (x2 - x1) * t;
                        float y = y1 + (y2 - y1) * t;

                        SDL_FRect pixel = {
                            x,
                            y,
                            2,
                            2
                        };
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
                else{
                    target -> hp -= tower -> damage;

                    if(tower -> slow_effect){
                        target -> speed = target -> base_speed / tower -> slow_multiplier;
                        target -> slow_timer = 4000;
                    }

                    if(target -> hp <= 0){
                        target -> active = 0;
                        coins += target -> reward;
                        score += target -> score;
                    }

                    tower -> shot_fired = 0;
                    tower -> cooldown_timer = tower -> cooldown;
                }
            }
            else if(tower -> cooldown_timer <= 0 && !tower -> shot_fired){
                tower -> shot_fired = 1;
                tower -> start_time = now;
            }
        }
    }

    /* menu pro věže */
    if(tower_menu_active){
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
        Tower* tower = &built_towers[tower_slot[selected_slot_index].tower_index];
        SDL_FRect tower_rect[4] = {tower_option1, tower_option2, tower_option3, tower_option4};

        char text[4][100];
        SDL_Color color = {255, 255, 255, 255};

        if(tower_type == -2){
            if(tower -> level < 3){
                sprintf(text[0], "Upgrade - %d", tower -> price * 2);
            }
            else{
                sprintf(text[0], "Maximální úroveň");
            }
            sprintf(text[1], "Exit");

            for(int i = 0; i < 2; i++){
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_FRect rect = tower_rect[i];

                SDL_RenderFillRect(renderer, &rect);

                /* zničení starých textur */
                if(option_texture[i]){
                    SDL_DestroyTexture(option_texture[i]);
                    option_texture[i] = NULL;
                }

                SDL_Surface* surface = TTF_RenderText_Solid(font, text[i], strlen(text[i]), color);
                option_texture[i] = SDL_CreateTextureFromSurface(renderer, surface);

                SDL_FRect text_rect;
                text_rect.x = rect.x + (rect.w - surface -> w) / 2;
                text_rect.y = rect.y + (rect.h - surface -> h) / 2;
                text_rect.w = surface -> w;
                text_rect.h = surface -> h;

                SDL_RenderTexture(renderer, option_texture[i], NULL, &text_rect);
                SDL_DestroySurface(surface);
            }
        }
        else{
            sprintf(text[0], "%s - %d", towers[0].name, towers[0].price);
            sprintf(text[1], "%s - %d", towers[1].name, towers[1].price);
            sprintf(text[2], "%s - %d", towers[2].name, towers[2].price);
            sprintf(text[3], "Exit");

            for(int i = 0; i < 4; i++){
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_FRect rect = tower_rect[i];

                SDL_RenderFillRect(renderer, &rect);

                /* zničení starých textur */
                if(option_texture[i]){
                    SDL_DestroyTexture(option_texture[i]);
                    option_texture[i] = NULL;
                }

                SDL_Surface* surface = TTF_RenderText_Solid(font, text[i], strlen(text[i]), color);
                option_texture[i] = SDL_CreateTextureFromSurface(renderer, surface);

                SDL_FRect text_rect;
                text_rect.x = rect.x + (rect.w - surface -> w) / 2;
                text_rect.y = rect.y + (rect.h - surface -> h) / 2;
                text_rect.w = surface -> w;
                text_rect.h = surface -> h;

                SDL_RenderTexture(renderer, option_texture[i], NULL, &text_rect);
                SDL_DestroySurface(surface);
            }
        }
    }

    UpdatePlayerHealth(renderer, font, health);

    if(hp_texture){
        SDL_FRect health_rect = {
            10,
            10,
            (float)health_width,
            (float)health_height
        };
        SDL_RenderTexture(renderer, hp_texture, NULL, &health_rect);
    }

    if(health == 0){
        SDL_Log("Uživatel byl poražen");
        return SDL_APP_SUCCESS;
    }

    UpdatePlayerCoins(renderer, font, coins);

    if(coins_texture){
        SDL_FRect coins_rect = {
            10,
            40,
            (float)coins_width,
            (float)coins_height
        };
        SDL_RenderTexture(renderer, coins_texture, NULL, &coins_rect);
    }

    UpdatePlayerScore(renderer, font, score);

    if(score_texture){
        SDL_FRect score_rect = {
            10,
            70,
            (float)score_width,
            (float)score_height
        };
        SDL_RenderTexture(renderer, score_texture, NULL, &score_rect);
    }
    
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    
    SDL_DestroyWindow(window);
    window = NULL;

    SDL_DestroyTexture(map_texture);

    SDL_DestroyTexture(monster);
    monster = NULL;

    SDL_DestroyTexture(tower);
    tower = NULL;

    SDL_DestroyTexture(hp_texture);
    hp_texture = NULL;

    SDL_DestroyTexture(coins_texture);
    coins_texture = NULL;

    SDL_DestroyTexture(score_texture);
    score_texture = NULL;

    TTF_Quit();
    SDL_Quit();
}