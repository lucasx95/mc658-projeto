#include <iostream>
#include <vector>
#include <queue>
#include <fstream>
#include <climits>

using namespace std;

/**
 * Actor data definition
 */
typedef struct Actor {
    int start = -1;
    int end = -1;
    int filmed_scenes = 0;
    unsigned long cost = 0;
    bool complete = false;
    int id = -1;
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
    vector<int> start_scenes;
    vector<int> end_scenes;
    vector<int> avaible_scenes;
    unsigned long cost;
    priority_queue<Solution> solutions;
    vector<Actor> actors;
    unsigned long possible_cost;

    bool operator<(const Solution &compareTo) const {
        return possible_cost + cost > compareTo.possible_cost + compareTo.cost;
    };

} Solution;

/**
 * Sets min/max initial cost and solves the problem
 */
unsigned long node_count = 0;
unsigned long max_cost = ULONG_MAX;
Solution best_solution;
unsigned long days_num_lkup;
unsigned long actors_num_lkup;
vector<vector<int>> actors_scenes_lkup;
vector<ActorLkup> actors_lkup;

void init_data(unsigned long days_num, unsigned long actors_num, vector<vector<int>> actors_scenes,
               vector<unsigned long> actors_cost) {
    days_num_lkup = days_num;
    actors_num_lkup = actors_num;
    actors_scenes_lkup = actors_scenes;
    for (int i = 0; i < actors_num; ++i) {
        ActorLkup actor_lkup;
        actor_lkup.cost = actors_cost[i];
        actor_lkup.total_scenes = 0;
        for (int j = 0; j < days_num; ++j) {
            if (actors_scenes_lkup[i][j]) actor_lkup.total_scenes++;
        }
        actors_lkup.push_back(actor_lkup);
    }
}

void solve(Solution solution) {
    node_count++;
    // verifies if it is a complete, it will only enter the function if solution.cost < max_cost
    if (solution.avaible_scenes.size() == 0) {
        cout << "New best soluction selected. Cost = " << solution.cost<< endl;
        max_cost = solution.cost;
        best_solution = solution;
        return;
    }

    // verifies if should insert start or end
    bool insert_start = solution.end_scenes.size() >= solution.start_scenes.size();
    // inserting on the end of the schedule
    for (int i = 0; i < solution.avaible_scenes.size(); ++i) {
        Solution child;
        if (insert_start) {
            // create child and calculate its cost
            child.end_scenes = solution.end_scenes;

            child.start_scenes = solution.start_scenes;
            child.start_scenes.push_back(solution.avaible_scenes[i]);

            child.avaible_scenes = solution.avaible_scenes;
            child.avaible_scenes.erase(child.avaible_scenes.begin() + i);

            child.actors = solution.actors;
            child.cost = 0;
            child.possible_cost = 0;
        } else {
            child.start_scenes = solution.start_scenes;

            child.end_scenes.push_back(solution.avaible_scenes[i]);
            child.end_scenes.insert(child.end_scenes.end(), solution.end_scenes.begin(), solution.end_scenes.end());

            child.avaible_scenes = solution.avaible_scenes;
            child.avaible_scenes.erase(child.avaible_scenes.begin() + i);

            child.actors = solution.actors;
            child.cost = 0;
            child.possible_cost = 0;
        }
        for (int j = 0; j < actors_num_lkup; ++j) {
            if (child.actors[j].complete) {
                child.cost += child.actors[j].cost;
                continue;
            }

            if (insert_start) {
                if (actors_scenes_lkup[j][solution.avaible_scenes[i]]) {
                    child.actors[j].filmed_scenes++;
                    if (child.actors[j].start == -1) {
                        child.actors[j].start = (int) child.start_scenes.size();
                    }
                    if (child.actors[j].end == -1 &&
                        !(actors_lkup[j].total_scenes - child.actors[j].filmed_scenes)) {
                        child.actors[j].end = (int) child.start_scenes.size();
                    }
                }
            } else {
                if (actors_scenes_lkup[j][solution.avaible_scenes[i]]) {
                    child.actors[j].filmed_scenes++;
                    if (child.actors[j].end == -1) {
                        child.actors[j].end = (int) (days_num_lkup - (int) child.end_scenes.size()) + 1;
                    }
                    if (child.actors[j].start == -1 && !(actors_lkup[j].total_scenes - child.actors[j].filmed_scenes)) {
                        child.actors[j].start = (int) (days_num_lkup - (int) child.end_scenes.size()) + 1;
                    }
                }
            }
            if (child.actors[j].start != -1) {
                if (child.actors[j].end == -1) {
                    child.actors[j].cost =
                            (((child.start_scenes.size() - child.actors[j].start) - child.actors[j].filmed_scenes) +
                             1) *
                            actors_lkup[j].cost;
                } else {
                    child.actors[j].cost =
                            (((child.actors[j].end - child.actors[j].start) - actors_lkup[j].total_scenes) + 1) *
                            actors_lkup[j].cost;
                    child.actors[j].complete = true;
                }
            } else {
                if (child.actors[j].end != -1) {
                    child.actors[j].cost =
                            ((child.actors[j].end - ((days_num_lkup - (int) child.end_scenes.size()) + 1)) -
                             child.actors[j].filmed_scenes + 1) * actors_lkup[j].cost;
                }
            }
            child.cost += child.actors[j].cost;
        }

        // calculate a min sum for the avaible scenes
        vector<int> open_start, open_end;
        for (int l = 0; l < actors_num_lkup; ++l) {
            if (child.actors[l].start != -1 && child.actors[l].end == -1) {
                open_start.push_back(l);
            } else if (child.actors[l].end != -1 && child.actors[l].start == -1) {
                open_end.push_back(l);
            }
        }

        priority_queue<OrderedScene> ordered_scenes_start, ordered_scenes_end;
        for (int k = 0; k < child.avaible_scenes.size(); ++k) {
            unsigned long start_cost = 0, end_cost = 0;
            for (int j = 0; j < open_start.size(); ++j) {
                if (actors_scenes_lkup[open_start[j]][child.avaible_scenes[k]]) {
                    start_cost += actors_lkup[open_start[j]].cost;
                }
            }

            for (int j = 0; j < open_end.size(); ++j) {
                if (actors_scenes_lkup[open_end[j]][child.avaible_scenes[k]]) {
                    start_cost += actors_lkup[open_end[j]].cost;
                }
            }

            OrderedScene start_scene, end_scene;
            start_scene.scene = end_scene.scene = child.avaible_scenes[k];
            start_scene.cost = start_cost;
            end_scene.cost = end_cost;
            ordered_scenes_start.push(start_scene);
            ordered_scenes_end.push(end_scene);
        }

        vector<bool> selected_start(actors_num_lkup), selected_end(actors_num_lkup);
        vector<int> selected_start_scenes, selected_end_scenes;
        for (int m = 0; m < child.avaible_scenes.size(); ++m) {
            bool valid_start_scene = false;
            for (int j = 0; j < open_start.size(); ++j) {
                if (actors_scenes_lkup[open_start[j]][ordered_scenes_start.top().scene]) {
                    if (selected_start[open_start[j]]) {
                        valid_start_scene = false;
                        break;
                    }
                    valid_start_scene = true;
                }
            }
            if (valid_start_scene) {
                selected_start_scenes.push_back(ordered_scenes_start.top().scene);
                for (int j = 0; j < open_start.size(); ++j) {
                    if (actors_scenes_lkup[open_start[j]][ordered_scenes_start.top().scene]) {
                        selected_start[open_start[j]] = true;
                    }
                }
            };
            ordered_scenes_start.pop();

            bool valid_end_scene = false;
            for (int j = 0; j < open_end.size(); ++j) {
                if (actors_scenes_lkup[open_end[j]][ordered_scenes_end.top().scene]) {
                    if (selected_end[open_end[j]]) {
                        valid_end_scene = false;
                        break;
                    }
                    valid_end_scene = true;
                }
            }
            if (valid_end_scene) {
                selected_end_scenes.push_back(ordered_scenes_end.top().scene);
                for (int j = 0; j < open_end.size(); ++j) {
                    if (actors_scenes_lkup[open_end[j]][ordered_scenes_end.top().scene]) {
                        selected_end[open_end[j]] = true;
                    }
                }
            };
            ordered_scenes_end.pop();
        }
        vector<Actor> estimated_start_scenes_actors_cost;
        for (int n = 0; n < open_start.size(); ++n) {
            Actor actor;
            actor.id = open_start[n];
            actor.cost = 0;
            actor.filmed_scenes = solution.actors[open_start[n]].filmed_scenes;
        }
        for (int n = 0; n < selected_start_scenes.size(); ++n) {
            for (int j = 0; j < estimated_start_scenes_actors_cost.size(); ++j) {
                if (actors_scenes_lkup[estimated_start_scenes_actors_cost[j].id][selected_start_scenes[n]]) {
                    estimated_start_scenes_actors_cost[j].filmed_scenes++;
                } else {
                    if (estimated_start_scenes_actors_cost[j].filmed_scenes <
                        actors_lkup[estimated_start_scenes_actors_cost[j].id].total_scenes) {
                        estimated_start_scenes_actors_cost[j].cost += actors_lkup[estimated_start_scenes_actors_cost[j].id].cost;
                    }
                }
            }
        }
        for (int n = 0; n < estimated_start_scenes_actors_cost.size(); ++n) {
            child.possible_cost += estimated_start_scenes_actors_cost[n].cost;
        }

        vector<Actor> estimated_end_scenes_actors_cost;
        for (int n = 0; n < open_end.size(); ++n) {
            Actor actor;
            actor.id = open_end[n];
            actor.cost = 0;
            actor.filmed_scenes = solution.actors[open_end[n]].filmed_scenes;
        }
        for (int n = 0; n < selected_end_scenes.size(); ++n) {
            for (int j = 0; j < estimated_end_scenes_actors_cost.size(); ++j) {
                if (actors_scenes_lkup[estimated_end_scenes_actors_cost[j].id][selected_end_scenes[n]]) {
                    estimated_end_scenes_actors_cost[j].filmed_scenes++;
                } else {
                    if (estimated_end_scenes_actors_cost[j].filmed_scenes <
                        actors_lkup[estimated_end_scenes_actors_cost[j].id].total_scenes) {
                        estimated_end_scenes_actors_cost[j].cost += actors_lkup[estimated_end_scenes_actors_cost[j].id].cost;
                    }
                }
            }
        }
        for (int n = 0; n < estimated_end_scenes_actors_cost.size(); ++n) {
            child.possible_cost += estimated_end_scenes_actors_cost[n].cost;
        }

        solution.solutions.push(child);
    }
    // explore sons
    while (!solution.solutions.empty() && solution.solutions.top().cost < max_cost) {
        cout << "New possible soluction selected." << solution.solutions.top().cost << endl;
        solve(solution.solutions.top());
        solution.solutions.pop();
    }
}

/**
 * Main function, organize the algorithm flow
 * @param argc num of arguments on the command line
 * @param argv argv[1] contains the path of the entry_file
 * @return 0 in case of success
 */
int main(int argc, const char *argv[]) {
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
    vector<vector<int>> actors_scenes(actors_num, vector<int>(days_num));
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

    cout << "Number of days: " << days_num << endl;
    cout << "Number of Actor: " << actors_num << endl;
    cout << "Scenes requirements: " << endl;
    for (int actor = 0; actor < actors_num; ++actor) {
        for (int day = 0; day < days_num; ++day) {
            cout << actors_scenes[actor][day] << " ";
        }
        cout << endl;
    }

    // init solving problem
    Solution empty_solution;
    empty_solution.cost = 0;
    for (int j = 0; j < days_num; ++j) {
        empty_solution.avaible_scenes.push_back(j);
    }
    empty_solution.actors = vector<Actor>(actors_num);
    init_data(days_num, actors_num, actors_scenes, actors_cost);
    // print actor lookup
    cout << "Actors" << endl;
    for (int k = 0; k < actors_num; ++k) {
        cout << "Number of scenes:  " << actors_lkup[k].total_scenes << " Cost: " << actors_lkup[k].cost << endl;
    }
    solve(empty_solution);

    cout << "Solution" << endl;

    for (int actor = 0; actor < actors_num; ++actor) {
        cout << "Start/End:" << best_solution.actors[actor].start << "/" << best_solution.actors[actor].end << " Cost: "
             << best_solution.actors[actor].cost << endl;
    }
    for (int actor = 0; actor < actors_num; ++actor) {
        for (int l = 0; l < best_solution.start_scenes.size(); ++l) {
            cout << actors_scenes[actor][best_solution.start_scenes[l]] << " ";
        }
        for (int l = 0; l < best_solution.end_scenes.size(); ++l) {
            cout << actors_scenes[actor][best_solution.end_scenes[l]] << " ";
        }

        cout << endl;
    }
    cout << "Formated solution" << endl;
    for (int l = 0; l < best_solution.start_scenes.size(); ++l) {
        cout << best_solution.start_scenes[l] << " ";
    }
    for (int l = 0; l < best_solution.end_scenes.size(); ++l) {
        cout << best_solution.end_scenes[l] << " ";
    }
    cout << endl << max_cost << endl;
    cout << node_count << endl;
    return 0;
}

