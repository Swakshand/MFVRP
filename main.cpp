#include<bits/stdc++.h>
#include<cstdio>
#include<cassert>
#include<utility>

using namespace std;
using DistanceMatrix = vector<vector<double>>;
using Penalty = vector<vector<int>>;
int ev_cost;
int cv_cost;
struct Customer{
    int demand;
    double x;
    double y;
    bool res;
};

struct Node{
    int index;
    int in;
    int out;
};

using Tour = vector<Node>;

struct Vehicle{
    int capacity;
    int available;
    Tour tour;
    double tour_length ;
    bool ev;
    int index;

    Vehicle(): capacity(0), available(0), tour(),tour_length(0),ev(false), index(-1) {}

    bool operator<(const Vehicle& other) const {
        return index < other.index;
    }
};

auto init_distance_matrix(const vector<Customer> & customers){
    auto square = [](auto x) { return x * x; };
    auto distance = [square](auto & a, auto & b) { return sqrt(square(a.x - b.x) + square(a.y - b.y)); };

    auto distance_matrix = DistanceMatrix(customers.size(), vector<double>(customers.size()));
    for(auto i = 0; i < customers.size(); ++i){
        for(auto j = 0; j < customers.size(); ++j){
            distance_matrix[i][j] = distance(customers[i], customers[j]);
        }
    }
    return distance_matrix;
}

auto correct_tour(Vehicle & vehicle, const DistanceMatrix & distance_matrix){
    auto & tour = vehicle.tour;
    for(auto i = 0; i < tour.size(); ++i){
        tour[i].in = tour[(i + tour.size() - 1) % tour.size()].index;
        tour[i].out = tour[(i + tour.size() + 1) % tour.size()].index;
    }
}

auto init_tour(const vector<Customer> & customers, vector<Vehicle> & vehicles, const DistanceMatrix & distance_matrix){
    auto non_served_customers_res = unordered_set<int>();
    auto non_served_customers_nres = unordered_set<int>();
    for(auto i = 1; i < customers.size(); ++i){
        if(customers[i].res){ 
        non_served_customers_res.insert(i);}
        else{
            non_served_customers_nres.insert(i);
        }
    }
    for(auto & vehicle : vehicles){
        vehicle.tour = Tour{ {0, 0, 0} };
    }

    auto vehicle = 0;
    while(!non_served_customers_res.empty()){
        while(true){
            auto max_demand = numeric_limits<int>::min();
            auto max_demand_customer = -1;

            for(auto i = 1; i < customers.size(); ++i){
                if(non_served_customers_res.find(i) == non_served_customers_res.end()) continue;
                if(max_demand < customers[i].demand && customers[i].demand <= vehicles[vehicle].available){
                    max_demand = customers[i].demand;
                    max_demand_customer = i;
                }
            }

            if(max_demand_customer == -1) break;

            vehicles[vehicle].available -= max_demand;
            vehicles[vehicle].tour.push_back({max_demand_customer, -1, -1});
            non_served_customers_res.erase(max_demand_customer);
        }
        vehicle += 1;
    }
    while(!non_served_customers_nres.empty()){
        while(true){
            auto max_demand = numeric_limits<int>::min();
            auto max_demand_customer = -1;

            for(auto i = 1; i < customers.size(); ++i){
                if(non_served_customers_nres.find(i) == non_served_customers_nres.end()) continue;
                if(max_demand < customers[i].demand && customers[i].demand <= vehicles[vehicle].available){
                    max_demand = customers[i].demand;
                    max_demand_customer = i;
                }
            }

            if(max_demand_customer == -1) break;

            vehicles[vehicle].available -= max_demand;
            vehicles[vehicle].tour.push_back({max_demand_customer, -1, -1});
            non_served_customers_nres.erase(max_demand_customer);
        }
        vehicle += 1;
    }

    for(auto & vehicle : vehicles){
        correct_tour(vehicle, distance_matrix);
    }
}



auto get_vehicle_cost(const Vehicle & vehicle, const DistanceMatrix & distance_matrix){
    auto cost = 0.0;
    for(auto node : vehicle.tour){
        cost += distance_matrix[node.index][node.out];
    }
    return cost;
}

auto get_cost(vector<Vehicle> & vehicles, const DistanceMatrix & distance_matrix,int n ,int m){
    auto cost = 0.0;
    for(auto & vehicle : vehicles){
        if(vehicle.ev){
            cost += get_vehicle_cost(vehicle, distance_matrix)*n;
        }
        else{ 
        cost += get_vehicle_cost(vehicle, distance_matrix)*m;
        }
    }
    return cost;
}



auto get_vehicle_augmented_cost(const Vehicle & vehicle, const DistanceMatrix & distance_matrix, double lambda, const Penalty & penalty,const vector<Customer> & customers){
    auto augmented_cost = 0.0;
    for(auto node : vehicle.tour){
        if((customers[node.index].res || customers[node.out].res) && !vehicle.ev){
            augmented_cost += (2*(distance_matrix[node.index][node.out]) + lambda * penalty[node.index][node.out]);
        }
        else{ 
        augmented_cost += distance_matrix[node.index][node.out] + lambda * penalty[node.index][node.out];}
    }
    return augmented_cost;
}

auto get_augmented_cost(vector<Vehicle> & vehicles, const DistanceMatrix & distance_matrix, double lambda, const Penalty & penalty,const vector<Customer> & customers){
    auto augmented_cost = 0.0;
    for(auto & vehicle : vehicles){
        augmented_cost += get_vehicle_augmented_cost(vehicle, distance_matrix, lambda, penalty,customers);
    }
    return augmented_cost;
}


auto add_1(Penalty & penalty, int i, int j){
    penalty[i][j] += 1;
    penalty[j][i] += 1;
}

auto save_result(const char * filename, double cost, const vector<Vehicle> & vehicles){
    auto f = fopen(filename, "w");
    fprintf(f, "%lf %d\n", cost, 0);
    for(auto & v : vehicles){
        for(auto node : v.tour){
            fprintf(f, "%d ", node.index);
        }
        fprintf(f, "0\n");
    }
    fclose(f);
}

auto init_lambda(double cost, const vector<Vehicle> & vehicles, double alpha){
    auto edge_count = 0;
    for(auto & vehicle : vehicles){
        if(vehicle.available == vehicle.capacity) continue;
        edge_count += vehicle.tour.size();
    }
    return alpha * cost / edge_count;
}


auto remove_node(Vehicle & vehicle, int node_index, const DistanceMatrix & distance_matrix, const vector<Customer> & customers){
    auto & tour = vehicle.tour;
    vehicle.available += customers[tour[node_index].index].demand;
    for(auto i = node_index + 1; i < tour.size(); ++i){
        tour[i - 1] = tour[i];
    }

    tour.pop_back();
    correct_tour(vehicle, distance_matrix);
}

auto insert_node(Vehicle & vehicle, int customer_index, int node_pos, const DistanceMatrix & distance_matrix, const vector<Customer> & customers){
    auto & tour = vehicle.tour;
    vehicle.available -= customers[customer_index].demand;
    tour.push_back({-1, -1, -1});
    for(auto i = tour.size() - 2; i > node_pos; --i){
        tour[i + 1] = tour[i];
    }

    tour[node_pos + 1] = Node{customer_index, -1, -1};
    correct_tour(vehicle, distance_matrix);
}

auto neighbor_relocate(const vector<Vehicle>& vehicles, const vector<Customer>& customers, 
            const DistanceMatrix& distance_matrix, const Penalty& penalty, double lambda){
    auto max_augmented_cost_gain = -numeric_limits<double>::infinity();
    auto max_cost_gain = -numeric_limits<double>::infinity();
    auto max_vehicle_new_a = Vehicle();
    auto max_vehicle_new_b = Vehicle();
    auto relocate_feasible = false;

   
    for(auto & vehicle_a : vehicles){
        
        if(true){
           
            for(auto & vehicle_b : vehicles){
               
                if(true){

                    if(vehicle_a.index == vehicle_b.index) continue;

                    auto & tour_a = vehicle_a.tour;
                    auto & tour_b = vehicle_b.tour;

                    for(auto node_index_a = 1; node_index_a < tour_a.size(); ++node_index_a){
                        auto customer_index = tour_a[node_index_a].index;
                        auto & customer = customers[customer_index];
                        if(customer.demand > vehicle_b.available) continue;

                        for(auto node_index_b = 0; node_index_b < tour_b.size(); ++node_index_b){
                            auto vehicle_new_a = vehicle_a;
                            auto vehicle_new_b = vehicle_b;
                            //if(vehicle_new_b.tour_length< distance_matrix[customers[customer_index]][customers[node_index_b]])
                            insert_node(vehicle_new_b, customer_index, node_index_b, distance_matrix, customers);
                            remove_node(vehicle_new_a, node_index_a, distance_matrix, customers);

                            auto augmented_cost_old = get_vehicle_augmented_cost(vehicle_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_new = get_vehicle_augmented_cost(vehicle_new_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_new_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_gain = augmented_cost_old - augmented_cost_new;

                            if(max_augmented_cost_gain >= augmented_cost_gain) continue;

                            auto cost_old = get_vehicle_cost(vehicle_a, distance_matrix) + get_vehicle_cost(vehicle_b, distance_matrix);
                            auto cost_new = get_vehicle_cost(vehicle_new_a, distance_matrix) + get_vehicle_cost(vehicle_new_b, distance_matrix);

                            auto cost_gain = cost_old - cost_new;

                            max_augmented_cost_gain = augmented_cost_gain;
                            max_cost_gain = cost_gain;
                            max_vehicle_new_a = move(vehicle_new_a);
                            max_vehicle_new_b = move(vehicle_new_b);
                            relocate_feasible = true;
                        }
                    }
                }
              
            }
        }
       
    }

    if(max_augmented_cost_gain < 1e-6){
        max_augmented_cost_gain = -numeric_limits<double>::infinity();
        max_cost_gain = -numeric_limits<double>::infinity();
        max_vehicle_new_a = Vehicle();
        max_vehicle_new_b = Vehicle();
        relocate_feasible = false;
    }

    return make_tuple(max_augmented_cost_gain, max_cost_gain, max_vehicle_new_a, max_vehicle_new_b, relocate_feasible);
}


auto neighbor_exchange(const vector<Vehicle> & vehicles, const vector<Customer> & customers, 
                const DistanceMatrix & distance_matrix, const Penalty & penalty, double lambda){
    auto max_augmented_cost_gain = -numeric_limits<double>::infinity();
    auto max_cost_gain = -numeric_limits<double>::infinity();
    auto max_vehicle_new_a = Vehicle();
    auto max_vehicle_new_b = Vehicle();
    auto exchange_feasible = false;

    for(auto & vehicle_a : vehicles){
      
        if(true){
           
            for(auto & vehicle_b : vehicles){
                
                if(true){

                    if(vehicle_a.index == vehicle_b.index) continue;
                    auto & tour_a = vehicle_a.tour;
                    auto & tour_b = vehicle_b.tour;

                    for(auto node_index_a = 1; node_index_a < tour_a.size(); ++node_index_a){
                        for(auto node_index_b = 1; node_index_b < tour_b.size(); ++node_index_b){
                            auto & customer_a = customers[tour_a[node_index_a].index];
                            auto & customer_b = customers[tour_b[node_index_b].index];

                            if(vehicle_a.available + customer_a.demand < customer_b.demand) continue;
                            if(vehicle_b.available + customer_b.demand < customer_a.demand) continue;

                            auto vehicle_new_a = vehicle_a;
                            auto vehicle_new_b = vehicle_b;

                            vehicle_new_a.available += customer_a.demand - customer_b.demand;
                            vehicle_new_b.available += customer_b.demand - customer_a.demand;

                            swap(vehicle_new_a.tour[node_index_a], vehicle_new_b.tour[node_index_b]);
                            correct_tour(vehicle_new_a, distance_matrix);
                            correct_tour(vehicle_new_b, distance_matrix);

                            auto augmented_cost_old = get_vehicle_augmented_cost(vehicle_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_new = get_vehicle_augmented_cost(vehicle_new_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_new_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_gain = augmented_cost_old - augmented_cost_new;

                            if(max_augmented_cost_gain >= augmented_cost_gain) continue;

                            auto cost_old = get_vehicle_cost(vehicle_a, distance_matrix) + get_vehicle_cost(vehicle_b, distance_matrix);
                            auto cost_new = get_vehicle_cost(vehicle_new_a, distance_matrix) + get_vehicle_cost(vehicle_new_b, distance_matrix);

                            auto cost_gain = cost_old - cost_new;

                            max_augmented_cost_gain = augmented_cost_gain;
                            max_cost_gain = cost_gain;
                            max_vehicle_new_a = move(vehicle_new_a);
                            max_vehicle_new_b = move(vehicle_new_b);
                            exchange_feasible = true;
                        }
                    }
                }
              
            }
        }
      
    }

    if(max_augmented_cost_gain < 1e-6){
        max_augmented_cost_gain = -numeric_limits<double>::infinity();
        max_cost_gain = -numeric_limits<double>::infinity();
        max_vehicle_new_a = Vehicle();
        max_vehicle_new_b = Vehicle();
        exchange_feasible = false;
    }

    return make_tuple(max_augmented_cost_gain, max_cost_gain, max_vehicle_new_a, max_vehicle_new_b, exchange_feasible);
}



auto neighbor_two_opt(const vector<Vehicle> & vehicles, const vector<Customer> & customers, 
                const DistanceMatrix & distance_matrix, const Penalty & penalty, double lambda){
    auto max_augmented_cost_gain = -numeric_limits<double>::infinity();
    auto max_cost_gain = -numeric_limits<double>::infinity();
    auto max_vehicle_new = Vehicle();
    auto two_opt_feasible = false;

    for(auto & vehicle : vehicles){

        if(true){
            
            auto & tour = vehicle.tour;
            for(auto t1 = 0; t1 < tour.size(); ++t1){
                auto t2 = (t1 + 1) % tour.size();
                
                for(auto t3 = 0; t3 < tour.size(); ++t3){
                    auto t4 = (t3 + 1) % tour.size();

                    if(t1 == t3 || t1 == t4 || t2 == t3 || t2 == t4) continue;

                    auto vehicle_new = vehicle;
                    auto & tour_new = vehicle_new.tour;
                    tour_new.clear();

                    tour_new.push_back(tour[t1]);
                    for(auto t = t3; t != t1; t = (t + tour.size() - 1) % tour.size()){
                        tour_new.push_back(tour[t]);
                    }

                    for(auto t = t4; t != t1; t = (t + 1) % tour.size()){
                        tour_new.push_back(tour[t]);
                    }

                    auto t = 0;
                    for(; t < tour_new.size(); ++t){
                        if(tour_new[t].index == 0) break;
                    }

                    auto tour_adjust_new = Tour();
                    for(auto i = 0; i < tour_new.size(); ++i){
                        tour_adjust_new.push_back(tour_new[t]);
                        t = (t + 1) % tour_new.size();
                    }

                    tour_new = tour_adjust_new;
                    correct_tour(vehicle_new, distance_matrix);

                    auto augmented_cost_old = get_vehicle_augmented_cost(vehicle, distance_matrix, lambda, penalty,customers);
                    auto augmented_cost_new = get_vehicle_augmented_cost(vehicle_new, distance_matrix, lambda, penalty,customers);

                    auto augmented_cost_gain = augmented_cost_old - augmented_cost_new;

                    if(max_augmented_cost_gain >= augmented_cost_gain) continue;

                    auto cost_old = get_vehicle_cost(vehicle, distance_matrix);
                    auto cost_new = get_vehicle_cost(vehicle_new, distance_matrix);

                    auto cost_gain = cost_old - cost_new;

                    max_augmented_cost_gain = augmented_cost_gain;
                    max_cost_gain = cost_gain;
                    max_vehicle_new = move(vehicle_new);
                    two_opt_feasible = true;
                }
            }
        }
        
    }

    if(max_augmented_cost_gain < 1e-6){
        max_augmented_cost_gain = -numeric_limits<double>::infinity();
        max_cost_gain = -numeric_limits<double>::infinity();
        max_vehicle_new = Vehicle();
        two_opt_feasible = false;
    }

    return make_tuple(max_augmented_cost_gain, max_cost_gain, max_vehicle_new, two_opt_feasible);
}



auto neighbor_cross(const vector<Vehicle> & vehicles, const vector<Customer> & customers, 
                const DistanceMatrix & distance_matrix, const Penalty & penalty, double lambda){
    auto max_augmented_cost_gain = -numeric_limits<double>::infinity();
    auto max_cost_gain = -numeric_limits<double>::infinity();
    auto max_vehicle_new_a = Vehicle();
    auto max_vehicle_new_b = Vehicle();
    auto cross_feasible = false;

    
    for(auto & vehicle_a : vehicles){
        
        if(true){
            
            for(auto & vehicle_b : vehicles){
                if(true){

                    if(vehicle_a.index == vehicle_b.index) continue;

                    auto & tour_a = vehicle_a.tour;
                    auto & tour_b = vehicle_b.tour;

                    for(auto node_index_a = 0; node_index_a < tour_a.size(); ++node_index_a){
                        for(auto node_index_b = 0; node_index_b < tour_b.size(); ++node_index_b){
                            auto demand_a = 0;
                            
                            for(auto i = node_index_a + 1; i < tour_a.size(); ++i){
                                demand_a += customers[tour_a[i].index].demand;
                            }   

                            auto demand_b = 0;
                            
                            for(auto i = node_index_b + 1; i < tour_b.size(); ++i){
                                demand_b += customers[tour_b[i].index].demand;
                            }

                            if(vehicle_a.available + demand_a < demand_b) continue;
                            if(vehicle_b.available + demand_b < demand_a) continue;

                            auto vehicle_new_a = vehicle_a;
                            auto vehicle_new_b = vehicle_b;

                            auto & tour_new_a = vehicle_new_a.tour;
                            auto & tour_new_b = vehicle_new_b.tour;

                            vehicle_new_a.available += demand_a - demand_b;
                            vehicle_new_b.available += demand_b - demand_a;

                            tour_new_a.resize(node_index_a + 1);
                            tour_new_b.resize(node_index_b + 1);

                            for(auto i = node_index_b + 1; i < tour_b.size(); ++i){
                                tour_new_a.push_back(tour_b[i]);
                            }   

                            for(auto i = node_index_a + 1; i < tour_a.size(); ++i){
                                tour_new_b.push_back(tour_a[i]);
                            }

                            correct_tour(vehicle_new_a, distance_matrix);
                            correct_tour(vehicle_new_b, distance_matrix);

                            auto augmented_cost_old = get_vehicle_augmented_cost(vehicle_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_new = get_vehicle_augmented_cost(vehicle_new_a, distance_matrix, lambda, penalty,customers) +
                                                    get_vehicle_augmented_cost(vehicle_new_b, distance_matrix, lambda, penalty,customers);

                            auto augmented_cost_gain = augmented_cost_old - augmented_cost_new;

                            if(max_augmented_cost_gain >= augmented_cost_gain) continue;

                            auto cost_old = get_vehicle_cost(vehicle_a, distance_matrix) + get_vehicle_cost(vehicle_b, distance_matrix);
                            auto cost_new = get_vehicle_cost(vehicle_new_a, distance_matrix) + get_vehicle_cost(vehicle_new_b, distance_matrix);

                            auto cost_gain = cost_old - cost_new;

                            max_augmented_cost_gain = augmented_cost_gain;
                            max_cost_gain = cost_gain;
                            max_vehicle_new_a = move(vehicle_new_a);
                            max_vehicle_new_b = move(vehicle_new_b);
                            cross_feasible = true;
                        }
                    }
                }

            }
        }

    }

    if(max_augmented_cost_gain < 1e-6){
        max_augmented_cost_gain = -numeric_limits<double>::infinity();
        max_cost_gain = -numeric_limits<double>::infinity();
        max_vehicle_new_a = Vehicle();
        max_vehicle_new_b = Vehicle();
        cross_feasible = false;
    }

    return make_tuple(max_augmented_cost_gain, max_cost_gain, max_vehicle_new_a, max_vehicle_new_b, cross_feasible);
}



auto add_penalty(Penalty & penalty, const vector<Vehicle> & vehicles, const DistanceMatrix & distance_matrix, double lambda, double & augmented_cost){
    auto max_util = -numeric_limits<double>::infinity();
    auto max_edge = vector<tuple<int, int>>();

    for(auto & vehicle : vehicles){
        auto & tour = vehicle.tour;

        for(auto node : tour){
            auto i = node.index;
            auto j = node.out;
            auto util = distance_matrix[i][j] / (1 + penalty[i][j]);

            if(max_util < util){
                max_util = util;
                max_edge.clear();
                max_edge.push_back(make_tuple(i, j));
            }
            else if(max_util == util){
                max_edge.push_back(make_tuple(i, j));
            }
        }
    }

    for(auto edge : max_edge){
        auto [i, j] = edge;
        add_1(penalty, i, j);
        augmented_cost += lambda;
    }
}



auto search(const vector<Customer> & customers, vector<Vehicle>& vehicles,int n, int m){
    auto distance_matrix = init_distance_matrix(customers);
    auto penalty = Penalty(distance_matrix.size(), vector<int>(distance_matrix.size(), 0));

    init_tour(customers, vehicles, distance_matrix);

    auto lambda = 0.0;
    auto alpha = 0.1;
    auto cost = get_cost(vehicles, distance_matrix,n,m);
    auto augmented_cost = get_augmented_cost(vehicles, distance_matrix, lambda, penalty,customers);
    auto best_cost = cost;
    auto best_vehicles = vehicles;

    auto step_limit = 10000;
    for(auto step = 0; step < 10000; ++step){
        printf("[Step %8d/%8d] [Lambda %lf] [Cost %lf] [Augmented Cost %lf] [Best Cost %lf]\n", 
                step + 1, step_limit, lambda, cost, augmented_cost, best_cost);

        auto [relocate_augmented_cost_gain, relocate_cost_gain, relocate_vehicle_new_a, relocate_vehicle_new_b, relocate_feasible] = 
                neighbor_relocate(vehicles, customers, distance_matrix, penalty, lambda);

        auto [exchange_augmented_cost_gain, exchange_cost_gain, exchange_vehicle_new_a, exchange_vehicle_new_b, exchange_feasible] = 
                neighbor_exchange(vehicles, customers, distance_matrix, penalty, lambda);

        auto [two_opt_augmented_cost_gain, two_opt_cost_gain, two_opt_vehicle_new, two_opt_feasible] =
                neighbor_two_opt(vehicles, customers, distance_matrix, penalty, lambda);

        auto [cross_augmented_cost_gain, cross_cost_gain, cross_vehicle_new_a, cross_vehicle_new_b, cross_feasible] =
                // neighbor_cross(vehicles, customers, distance_matrix, penalty, lambda, is_vehicle_serving_residential);
                neighbor_cross(vehicles, customers, distance_matrix, penalty, lambda);

        if(!relocate_feasible && !exchange_feasible && !two_opt_feasible && !cross_feasible){
            if(lambda == 0.0) lambda = init_lambda(cost, vehicles, alpha);
            add_penalty(penalty, vehicles, distance_matrix, lambda, augmented_cost);
        }
        else if(relocate_augmented_cost_gain >= exchange_augmented_cost_gain && relocate_augmented_cost_gain >= two_opt_augmented_cost_gain &&
                relocate_augmented_cost_gain >= cross_augmented_cost_gain){
            augmented_cost -= relocate_augmented_cost_gain;
            cost -= relocate_cost_gain;

            vehicles[relocate_vehicle_new_a.index] = move(relocate_vehicle_new_a);
            vehicles[relocate_vehicle_new_b.index] = move(relocate_vehicle_new_b);
        }
        else if(exchange_augmented_cost_gain >= relocate_augmented_cost_gain && exchange_augmented_cost_gain >= two_opt_augmented_cost_gain &&
                exchange_augmented_cost_gain >= cross_augmented_cost_gain){
            augmented_cost -= exchange_augmented_cost_gain;
            cost -= exchange_cost_gain;

            vehicles[exchange_vehicle_new_a.index] = move(exchange_vehicle_new_a);
            vehicles[exchange_vehicle_new_b.index] = move(exchange_vehicle_new_b);
        }
        else if(two_opt_augmented_cost_gain >= relocate_augmented_cost_gain && two_opt_augmented_cost_gain >= exchange_augmented_cost_gain &&
                two_opt_augmented_cost_gain >= cross_augmented_cost_gain){
            augmented_cost -= two_opt_augmented_cost_gain;
            cost -= two_opt_cost_gain;

            vehicles[two_opt_vehicle_new.index] = move(two_opt_vehicle_new);
        }
        else{
            augmented_cost -= cross_augmented_cost_gain;
            cost -= cross_cost_gain;

            vehicles[cross_vehicle_new_a.index] = move(cross_vehicle_new_a);
            vehicles[cross_vehicle_new_b.index] = move(cross_vehicle_new_b);
        }

        if(best_cost > cost){
            best_cost = cost;
            best_vehicles = vehicles;
            save_result("output_cpp.txt", best_cost, best_vehicles);
        }
    }

 

    save_result("output_cpp.txt", best_cost, best_vehicles);
}



int main() {
    int n_customer_ev;
    int n_customer_cv;
    int n_vehicle_ev;
    int n_vehicle_cv;
    int capacity_ev;
    int capacity_cv;
    float n;
    float m;
    int ev_max_tour;
    int cv_max_tour;
    // bool res;

    cout << "Enter n_customer_ev, n_customer_cv, n_vehicle_cv, n_vehicle_ev, capacity_ev, capacity_cv, ev_cost, cv_cost, ev_max_tour,cv_max_tour: "<<endl;
    cin >> n_customer_ev>>n_customer_cv >> n_vehicle_cv>>n_vehicle_ev >> capacity_ev>>capacity_cv >> n >> m>> ev_max_tour>>cv_max_tour;
    vector<Customer> customers(n_customer_cv + n_customer_ev);
    // vector<Customer> customers_ev;
    ev_cost = n;
    cv_cost = m;
    cout << "Enter customer demand and coordinates (x, y) and is_residential for each customer: " <<endl;
    for (auto &customer : customers) {
        int a,b,c;
        bool d;
        cin >> a >> b >> c>> d;
        customer.demand = c, customer.x = a, customer.y = b, customer.res = d;
        // if(d){
        //     customers_ev.push_back(customer);
        // }
    }

    vector<Vehicle> vehicles;
    vector<Vehicle> vehicles_ev;
    vector<Vehicle> remaining_vehicles;
    for (int i = 0; i < n_vehicle_ev; i++) {
        Vehicle vehicle;
        vehicle.capacity = capacity_ev;
        vehicle.available = capacity_ev;
        vehicle.index = i;
        vehicle.ev = true;
        vehicles.push_back(vehicle);
        // remaining_vehicles.push_back(vehicle);
    }
    for (int i = n_vehicle_ev; i < n_vehicle_ev + n_vehicle_cv; i++) {
        Vehicle vehicle;
        vehicle.capacity = capacity_cv;
        vehicle.available = capacity_cv;
        vehicle.index = i;
        vehicle.ev = false;
        vehicles.push_back(vehicle);
        // vehicles_ev.push_back(vehicle);
    }
    search(customers, vehicles,n,m);
 
    return 0;
}
