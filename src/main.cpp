#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

int random_integer(const int min, const int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

bool isPositionEmpty(const std::vector<std::vector<entity_t>>& board, int x, int y) {
    if (x < 0 || x >= board.size() || y < 0 || y >= board[x].size()) {
        return false;
    }
    return board[x][y].type == empty;
}

std::pair<int, int> emptyPosition(std::vector<std::vector<entity_t>>& board, int x, int y) {
    if (!isPositionEmpty(board, x, y)) {
        std::vector<std::pair<int, int>> emptyNeighbors;
        
        // Verifique as posições ao redor (acima, abaixo, esquerda, direita)
        if (isPositionEmpty(board, x - 1, y)) {
            emptyNeighbors.emplace_back(x - 1, y);
        }
        if (isPositionEmpty(board, x + 1, y)) {
            emptyNeighbors.emplace_back(x + 1, y);
        }
        if (isPositionEmpty(board, x, y - 1)) {
            emptyNeighbors.emplace_back(x, y - 1);
        }
        if (isPositionEmpty(board, x, y + 1)) {
            emptyNeighbors.emplace_back(x, y + 1);
        }
        
        // Se houver posições vazias, escolha uma aleatoriamente
        if (!emptyNeighbors.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, emptyNeighbors.size() - 1);
            int randomIndex = dist(gen);
            
            // Mova o elemento para a posição escolhida
            std::pair<int, int> newPosition = emptyNeighbors[randomIndex];
            return newPosition;
        }
    }
    std::pair<int, int> error;
    error.first = -1;
    error.second = -1;
    return error;
}

int main()
{
    entity_type_t types;
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }
        int num_plants = (uint32_t)request_body["plants"];
        int num_herb = (uint32_t)request_body["herbivores"];
        int num_carn = (uint32_t)request_body["carnivores"];

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        for (int i = 0; i < num_plants; ++i) {
        int x, y;
        do {
            x = random_integer(0, 14);
            y = random_integer(0, 14);
        } while (entity_grid[x][y].type != empty);
        entity_grid[x][y].type = plant;
        entity_grid[x][y].age = 0;
        }

         for (int i = 0; i < num_herb; ++i) {
        int x, y;
        do {
            x = random_integer(0, 14);
            y = random_integer(0, 14);
        } while (entity_grid[x][y].type != empty);
        entity_grid[x][y].type = herbivore;
        entity_grid[x][y].age = 0;
         entity_grid[x][y].energy = 100;
        }

        for (int i = 0; i < num_carn; ++i) {
        int x, y;
        do {
            x = random_integer(0, 14);
            y = random_integer(0, 14);
        } while (entity_grid[x][y].type != empty);
        entity_grid[x][y].type = carnivore;
        entity_grid[x][y].age = 0;
        entity_grid[x][y].energy = 100;
        }
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
         for(int i=0; i<15; i++)
         {
            for(int j=0; j<15; j++)
            {
                if(entity_grid[i][j].type == plant)
                {
                if (entity_grid[i][j].age >= PLANT_MAXIMUM_AGE) {
                // Plant has reached maximum age, it dies
                entity_grid[i][j].type = empty;
                entity_grid[i][j].age = 0;
            } else {
                entity_grid[i][j].age++;

                    if(random_action(PLANT_REPRODUCTION_PROBABILITY))
                    {
                         // Create a new plant in an adjacent empty cell if available
                    std::vector<pos_t> empty_adjacent_cells;

                    // Check cells to the north, south, east, and west for emptiness
                    if (i > 0 && entity_grid[i - 1][j].type == empty) {
                        empty_adjacent_cells.push_back({i - 1, j});

                    }
                    if (i < NUM_ROWS - 1 && entity_grid[i + 1][j].type == empty) {
                        empty_adjacent_cells.push_back({i + 1, j}); // South
                    }
                    if (j > 0 && entity_grid[i][j - 1].type == empty) {
                        empty_adjacent_cells.push_back({i, j - 1}); // West
                    }
                    if (j < NUM_ROWS - 1 && entity_grid[i][j + 1].type == empty) {
                        empty_adjacent_cells.push_back({i, j + 1}); // East
                    }

                    if (!empty_adjacent_cells.empty()) {
                        // Randomly select an empty adjacent cell
                        int random_index = random_integer(0,  (empty_adjacent_cells.size()-1));
                        pos_t new_position = empty_adjacent_cells[random_index];

                        // Create a new plant in the selected cell
                        entity_grid[new_position.i][new_position.j] = {plant, MAXIMUM_ENERGY, 0};
                    }
                }
            }
                    }
                
                if(entity_grid[i][j].type == herbivore)
                {
                    if(entity_grid[i][j].age >= HERBIVORE_MAXIMUM_AGE || entity_grid[i][j].energy<=0)
                    {

                    entity_grid[i][j].type = empty;
                    entity_grid[i][j].age = 0;
                    }
                    else{
                    entity_grid[i][j].age++;
                    std::vector<pos_t> adjacent_cells;
                    std::vector<pos_t> empty_adjacent_cells;
                    std::vector<pos_t> plant_adjacent_cells;
                    std::vector<pos_t> carnivore_adjacent_cells;


                    // Check cells to the north, south, east, and west for emptiness
                    if (i > 0) {
                       adjacent_cells.push_back({i - 1, j});

                    }
                    if (i < NUM_ROWS - 1) {
                       adjacent_cells.push_back({i + 1, j}); // South
                    }
                    if (j > 0) {
                        adjacent_cells.push_back({i, j - 1}); // West
                    }
                    if (j < NUM_ROWS - 1) {
                        adjacent_cells.push_back({i, j + 1}); // East
                    }
                    for (int f=0; f<adjacent_cells.size(); f++)
                    {
                        if(entity_grid[adjacent_cells[f].i][adjacent_cells[f].j] == empty)
                        {
                            empty_adjacent_cells.push_back(adjacent_cells[f]);
                        } else if(entity_grid[adjacent_cells[f].i][adjacent_cells[f].j]==plant){
                            plant_adjacent_cells.push_back(adjacent_cells[f]);
                        } else if(entity_grid[adjacent_cells[f].i][adjacent_cells[f].j]==carnivore)
                        {
                            carnivore_adjacent_cells.push_back(adjacent_cells[f]);
                        }
                    }

                    if(random_action(HERBIVORE_MOVE_PROBABILITY) && empty_adjacent_cells.size() > 0)
                    {
                        int random_index = random_integer(0,  (empty_adjacent_cells.size()-1));
                        pos_t new_position = empty_adjacent_cells[random_index];
                        entity_grid[new_position.i][new_position.j] = {herbivore, MAXIMUM_ENERGY, 0};
                        entity_grid[i][j].type = empty;
                        //empty_adjacent_cells.erase(new_position);
                    }
                    if(random_action(HERBIVORE_EAT_PROBABILITY) && plant_adjacent_cells.size() > 0)
                    {
                         int random_index = random_integer(0,  (empty_adjacent_cells.size()-1));
                        pos_t new_position = empty_adjacent_cells[random_index];
                        entity_grid[new_position.i][new_position.j] = {empty, 0, 0};
                        entity_grid[i][j].energy+=30; 
                    }
                    if(random_action(HERBIVORE_REPRODUCTION_PROBABILITY) && empty_adjacent_cells.size() > 0 && entity_grid[i][j].energy > 20)
                    {
                        int random_index = random_integer(0,  (empty_adjacent_cells.size()-1));
                        pos_t new_position = empty_adjacent_cells[random_index];
                        entity_grid[new_position.i][new_position.j] = {herbivore, MAXIMUM_ENERGY, 0};
                        //empty_adjacent_cells.erase(new_position);
                        entity_grid[i][j].energy-=10;
                    }

                    entity_grid[i][j].age++;
                    
                    }}

                if(entity_grid[i][j].type == carnivore)
                {
                    entity_grid[i][j].age++;
                    
                }
            }
         }
            
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); 
        });
    app.port(8080).run();

    return 0;
}