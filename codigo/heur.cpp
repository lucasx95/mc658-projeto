#include <iostream>
#include <cstdlib>
#include <vector>
#include <csignal>
#include <queue>
#include <fstream>
#include <climits>
#include <algorithm>
#include <numeric>

using namespace std;

/**
 * Actor data definition
 */
typedef struct Actor {
    int start;
    int filmed_scenes;
    bool complete;
} Actor;

/**
 * ActorLkup data definition
 */
typedef struct ActorLkup {
    int total_scenes;
    unsigned long cost;
} ActorLkup;

/**
 * OrderedScene by cost desc
 */
typedef struct OrderedScene {
    int scene;
    unsigned long cost;

    bool operator<(const OrderedScene &compareTo) const {
        return cost < compareTo.cost;
    }
} OrderedScene;
/**
 * A solution
 */
typedef struct Solution {
    vector<int> scenes;
    unsigned long cost;
    bool operator<(const Solution &compareTo) const {
        return cost < compareTo.cost;
    };
} Solution;

/**
 * Sets min/max initial cost and solves the problem
 */
unsigned long days_num_lkup;
vector<vector<int> > actors_scenes_lkup;
vector<int> scenes_sample;
vector<ActorLkup> actors_lkup;
vector<Solution> solutions;
vector<Actor> actors_sample;
bool best_solution_updating = false;
bool should_stop = false;
unsigned int  pop_size = 1000;
unsigned int block_size = 100;
unsigned int without_change = 0;
unsigned int without_change_limit = 1000000;

unsigned long calculate_cost(vector<int> &scene_order) {
    vector<Actor> actors = actors_sample;
    unsigned long cost = 0;
    for (int j = 0; j < scene_order.size(); ++j) {
        for (int k =0; k < actors_sample.size(); ++k) {
            if (actors_scenes_lkup[k][scene_order[j]]) {
                actors[k].filmed_scenes++;
                if (actors[k].start == -1) {
                    actors[k].start = scene_order[j];
                }
                if (actors[k].filmed_scenes == actors_lkup[k].total_scenes) {
                    actors[k].complete = true;
                }
            } else if (actors[k].start != -1 && !actors[k].complete) {
                cost += actors_lkup[k].cost;
            }
        }
    }
    return cost;
}

Solution generate_random_solution() {
    Solution random_solution;
    random_solution.scenes = scenes_sample;
    random_shuffle(random_solution.scenes.begin(),random_solution.scenes.end());
    random_solution.cost = calculate_cost(random_solution.scenes);
    return random_solution;
}

void generate_random_solutions() {
    while (solutions.size() < pop_size) {
        Solution random_solution = generate_random_solution();
        solutions.insert(std::upper_bound( solutions.begin(), solutions.end(), random_solution ),random_solution);
    }
}

void init_data(unsigned long days_num, unsigned long actors_num, vector<vector<int> > actors_scenes,
               vector<unsigned long> actors_cost) {
    days_num_lkup = days_num;
    actors_scenes_lkup = actors_scenes;
    scenes_sample.resize(days_num);
    for(int i = 0; i < days_num; i++) {
        scenes_sample[i] = i;
    }
    actors_sample.resize(actors_num);
    for (int k = 0; k < actors_num; ++k) {
        actors_sample[k].start = -1;
        actors_sample[k].filmed_scenes = 0;
        actors_sample[k].complete = false;
    }
    for (int i = 0; i < actors_num; ++i) {
        ActorLkup actor_lkup;
        actor_lkup.cost = actors_cost[i];
        actor_lkup.total_scenes = 0;
        for (int j = 0; j < days_num; ++j) {
            if (actors_scenes_lkup[i][j]) actor_lkup.total_scenes++;
        }
        actors_lkup.push_back(actor_lkup);
    }
    generate_random_solutions();
}

void print_formatted_result() {
    cout << endl;
    for (int l = 0; l < solutions[0].scenes.size(); ++l) {
        cout << solutions[0].scenes[l] << " ";
    }
    cout << endl << solutions[0].cost << endl;
}

unsigned int adaptative_proportional_position() {
    int block_number = pop_size / block_size;
    int block_probability = 100/block_number;
    unsigned int position = rand() % block_size;
    for(int i = 0; i < block_number - 1; i++) {
        position += (rand() % 100 < block_probability) ? block_size : 0;
    }
    return position;
}

void solve() {
    while (without_change < without_change_limit) {
        int position = adaptative_proportional_position();
        Solution chromosome = solutions[position];
        int mutation_size = (int) (((rand() % (days_num_lkup / 2))));
        for (int i = 0; i < mutation_size; i++) {
            int new_position = (int) (rand() % days_num_lkup);
            int scene_on_change = chromosome.scenes[i];
            chromosome.scenes[i] = chromosome.scenes[new_position];
            chromosome.scenes[new_position] = scene_on_change;
        }
        chromosome.cost = calculate_cost(chromosome.scenes);
        if (chromosome.cost < solutions[0].cost) {
            best_solution_updating = true;
            without_change = 0;
        } else {
            without_change++;
        }
        solutions.pop_back();
        solutions.insert(std::upper_bound(solutions.begin(), solutions.end(), chromosome), chromosome);
        best_solution_updating = false;
        if (should_stop) {
            break;
        }
    }
    print_formatted_result();
    exit(0);
}

void stop_execution(int signum) {
    should_stop = true;
    if(!best_solution_updating) {
        print_formatted_result();
        exit(0);
    }
}


/**
 * Main function, organize the algorithm flow
 * @param argc num of arguments on the command line
 * @param argv argv[1] contains the path of the entry_file
 * @return 0 in case of success
 */
int main(int argc, const char *argv[]) {
    // register signal
    signal(SIGINT, stop_execution);
    // data declaration
    unsigned long days_num, actors_num;

    // open file on argv[1]
    ifstream entry_file;
    entry_file.open(argv[1]);
    if (!entry_file) {
        cerr << "Não foi possível abrir o arquivo de entrada " << argv[1];
        exit(1);
    }

    // read data from file
    entry_file >> days_num;
    entry_file >> actors_num;

    // read scenes requirements
    vector<vector<int> > actors_scenes(actors_num, vector<int>(days_num));
    for (int actor = 0; actor < actors_num; ++actor) {
        for (int day = 0; day < days_num; ++day) {
            entry_file >> actors_scenes[actor][day];
        }
    }

    // read actor cost
    vector<unsigned long> actors_cost(actors_num);
    for (int i = 0; i < actors_num; ++i) {
        entry_file >> actors_cost[i];
    }
    // init solving problem
    init_data(days_num, actors_num, actors_scenes, actors_cost);
    solve();
    print_formatted_result();
    return 0;
}