#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import math
import copy
from types import SimpleNamespace
import vrplib
import random
import pandas as pd
import numpy as np
import numpy.random as rnd
from collections import defaultdict
from alns.accept import RecordToRecordTravel, SimulatedAnnealing, HillClimbing
from alns import ALNS, State
from alns.accept import HillClimbing
from alns.stop import MaxIterations
from alns.select import RouletteWheel
import matplotlib.pyplot as plt
get_ipython().run_line_magic('matplotlib', 'inline')
SEED = 1234

n_customer_ev = int(input("Enter the number of residential customers: "))
n_customer_cv = int(input("Enter the number of non-residential customers: "))
n_vehicle_ev = int(input("Enter the number of EVs: "))
n_vehicle_cv = int(input("Enter the number of CVs: "))
capacity_ev = int(input("Enter the capacity of EVs: "))
capacity_cv = int(input("Enter the capacity of CVs: "))
ev_cost = float(input("Enter the cost per unit distance of EVs: "))
cv_cost = float(input("Enter the cost per unit distance of CVs: "))
ev_max_tour = float(input("Enter the maximum tour length for EVs: "))
cv_max_tour = float(input("Enter the maximum tour length for CVs: "))

data = {}

def plot_routes(solution):
    plt.figure(figsize=(10, 10))
    colors = plt.cm.get_cmap("tab10", len(solution.routes))
    for idx, route in enumerate(solution.routes):
        x_coords = [data["customers"][customer][0] for customer in route]
        y_coords = [data["customers"][customer][1] for customer in route]
        plt.plot(x_coords, y_coords, marker='o', color=colors(idx), label=f"Vehicle {idx + 1}")
        for customer in route:
            x, y = data["customers"][customer][0], data["customers"][customer][1]
            plt.text(x, y, str(customer), fontsize=9, ha='right')

    plt.plot(data["customers"][0][0], data["customers"][0][1], 'ks', markersize=10, label="Depot")
    plt.xlabel("X coordinate")
    plt.ylabel("Y coordinate")
    plt.title("Vehicle Routes")
    plt.legend()
    plt.grid()
    plt.show()

class Solution:
    def __init__(self, routes, unassigned_customers, route_types):
        self.routes = routes
        self.unassigned = unassigned_customers
        self.route_types = route_types

    def copy(self):
        return Solution([route[:] for route in self.routes], self.unassigned[:], self.route_types[:])
    
    def find_route(self, customer):
        for route in self.routes:
            if customer in route[1:-1]:
                return route
        return None

    def objective(self):
        ev_distance, cv_distance = 0.0, 0.0
        for route, vehicle_type in zip(self.routes, self.route_types):
            distance = calculate_route_distance(route)
            if vehicle_type == "EV":
                ev_distance += distance
            else:
                cv_distance += distance
        total_cost = (ev_distance * ev_cost) + (cv_distance * cv_cost)
        return total_cost


def calculate_distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)

def calculate_route_distance(route):
    distance = 0.0
    for i in range(len(route) - 1):
        distance += data["distance_matrix"][route[i]][route[i + 1]]
    return distance

def initialize_data():
    data.update({
        "num_customers": n_customer_ev + n_customer_cv,
        "ev_count": n_vehicle_ev,
        "cv_count": n_vehicle_cv,
        "ev_capacity": capacity_ev,
        "cv_capacity": capacity_cv,
        "customers": [],
        "distance_matrix": [],
        "residential": []
    })
    
    data["customers"].append((0, 0, 0, 0))  
    for i in range(1, data["num_customers"] + 1):
        x, y, demand, residential = map(int, input(f"Customer {i} (x y demand residential): ").split())
        data["customers"].append((x, y, demand, residential))
        data["residential"].append(residential)
    data["distance_matrix"] = [[0.0] * (data["num_customers"] + 1) for _ in range(data["num_customers"] + 1)]
    for i in range(data["num_customers"] + 1):
        for j in range(data["num_customers"] + 1):
            data["distance_matrix"][i][j] = calculate_distance(
                data["customers"][i][0], data["customers"][i][1],
                data["customers"][j][0], data["customers"][j][1]
            )

# def create_initial_solution():
#     routes = []
#     route_types = []
#     ev_cap = data["ev_capacity"]
#     cv_cap = data["cv_capacity"]
#     residential_customers = [i for i in range(1, data["num_customers"] + 1) if data["customers"][i][3] == 1]
#     non_residential_customers = [i for i in range(1, data["num_customers"] + 1) if data["customers"][i][3] == 0]
#     unassigned_evs = []
#     assigned_customers = set() 

#     def route_builder(vehicle_count, capacity, customer_list, vehicle_type="EV"):
#         customer_index = 0
#         for _ in range(vehicle_count):
#             route = [0]
#             load = 0
#             while customer_index < len(customer_list) and load + data["customers"][customer_list[customer_index]][2] <= capacity:
#                 customer = customer_list[customer_index]
#                 if customer not in assigned_customers:
#                     route.append(customer)
#                     load += data["customers"][customer][2]
#                     assigned_customers.add(customer)
#                 customer_index += 1
#             route.append(0)
#             if len(route) > 2:
#                 routes.append(route)
#                 route_types.append(vehicle_type)
#             else:
#                 if vehicle_type == "EV":
#                     unassigned_evs.append(route)
    
#     route_builder(data["ev_count"], ev_cap, residential_customers, vehicle_type="EV")
#     remaining_non_residential_customers = [c for c in non_residential_customers if c not in assigned_customers]
#     route_builder(len(unassigned_evs), ev_cap, remaining_non_residential_customers, vehicle_type="EV")
#     remaining_non_residential_customers = [c for c in non_residential_customers if c not in assigned_customers]
#     route_builder(data["cv_count"], cv_cap, remaining_non_residential_customers, vehicle_type="CV")
#     return Solution(routes, [], route_types)
# Modify create_initial_solution to respect max tour length for each vehicle type
def create_initial_solution():
    routes = []
    route_types = []
    ev_cap = data["ev_capacity"]
    cv_cap = data["cv_capacity"]
    residential_customers = [i for i in range(1, data["num_customers"] + 1) if data["customers"][i][3] == 1]
    non_residential_customers = [i for i in range(1, data["num_customers"] + 1) if data["customers"][i][3] == 0]
    assigned_customers = set()

    def route_builder(vehicle_count, capacity, customer_list, vehicle_type="EV", max_tour_length=0):
        customer_index = 0
        for _ in range(vehicle_count):
            route = [0]
            load = 0
            while customer_index < len(customer_list) and load + data["customers"][customer_list[customer_index]][2] <= capacity:
                customer = customer_list[customer_index]
                proposed_route = route + [customer, 0]  # Tentatively add to check distance
                if customer not in assigned_customers and calculate_route_distance(proposed_route) <= max_tour_length:
                    route.append(customer)
                    load += data["customers"][customer][2]
                    assigned_customers.add(customer)
                customer_index += 1
            route.append(0)
            if len(route) > 2:
                routes.append(route)
                route_types.append(vehicle_type)

    route_builder(data["ev_count"], ev_cap, residential_customers, vehicle_type="EV", max_tour_length=ev_max_tour)
    remaining_non_residential_customers = [c for c in non_residential_customers if c not in assigned_customers]
    route_builder(len(residential_customers), ev_cap, remaining_non_residential_customers, vehicle_type="EV", max_tour_length=ev_max_tour)
    remaining_non_residential_customers = [c for c in non_residential_customers if c not in assigned_customers]
    route_builder(data["cv_count"], cv_cap, remaining_non_residential_customers, vehicle_type="CV", max_tour_length=cv_max_tour)
    return Solution(routes, [], route_types)

def random_removal(state, rng):
    destroyed = state.copy()
    degree_of_destruction = 0.05
    customers_to_remove = int(data["num_customers"] * degree_of_destruction)
    for customer in rng.choice(range(1, data["num_customers"] + 1), customers_to_remove, replace=False):
        destroyed.unassigned.append(customer)
        route = destroyed.find_route(customer)
        if route:
            route.remove(customer)
    return remove_empty_routes(destroyed)

def remove_empty_routes(state):
    state.routes = [route for route in state.routes if len(route) > 2]
    return state

# def greedy_repair(state, rng):
#     repaired = state.copy()
#     unassigned_customers = repaired.unassigned[:]
    
#     for customer in unassigned_customers:
#         is_residential = data["residential"][customer - 1] == 1
#         best_route, best_position, best_cost = None, None, float("inf")
#         eligible_routes = []
#         # Determine eligible routes based on customer type and route type
#         for route, vehicle_type in zip(repaired.routes, repaired.route_types):
#             if vehicle_type == "EV" or (vehicle_type == "CV" and not is_residential):
#                 eligible_routes.append((route, vehicle_type))
#         # Now find the best insertion position within eligible routes
#         for route, vehicle_type in eligible_routes:
#             for position in range(1, len(route)):
#                 new_route = route[:position] + [customer] + route[position:]
#                 new_cost = calculate_route_distance(new_route)
#                 if new_cost < best_cost:
#                     best_route, best_position, best_cost = route, position, new_cost
#         # Insert the customer into the best position found
#         if best_route is not None:
#             best_route.insert(best_position, customer)
#             repaired.unassigned.remove(customer)
#     return repaired
# Update greedy_repair to respect max tour lengths
def greedy_repair(state, rng):
    repaired = state.copy()
    unassigned_customers = repaired.unassigned[:]
    
    for customer in unassigned_customers:
        is_residential = data["residential"][customer - 1] == 1
        best_route, best_position, best_cost = None, None, float("inf")
        eligible_routes = []
        for route, vehicle_type in zip(repaired.routes, repaired.route_types):
            max_tour_length = ev_max_tour if vehicle_type == "EV" else cv_max_tour
            if vehicle_type == "EV" or (vehicle_type == "CV" and not is_residential):
                eligible_routes.append((route, vehicle_type, max_tour_length))
        for route, vehicle_type, max_length in eligible_routes:
            for position in range(1, len(route)):
                new_route = route[:position] + [customer] + route[position:]
                new_cost = calculate_route_distance(new_route)
                if new_cost < best_cost and new_cost <= max_length:
                    best_route, best_position, best_cost = route, position, new_cost

        if best_route is not None:
            best_route.insert(best_position, customer)
            repaired.unassigned.remove(customer)
    return repaired

def display_solution(solution):
    vehicle_counter = defaultdict(int)
    for vehicle_index, route in enumerate(solution.routes):
        if all(data["residential"][customer - 1] == 1 for customer in route[1:-1]):
            vehicle_counter["EV"] += 1
            distance = calculate_route_distance(route)
            print(f"Vehicle Type: EV")
            print(f"  Vehicle {vehicle_counter['EV']} Route: {route}")
            print(f"  Vehicle {vehicle_counter['EV']} Distance Covered: {distance:.2f}")
        else:
            vehicle_counter["CV"] += 1
            distance = calculate_route_distance(route)
            print(f"Vehicle Type: CV")
            print(f"  Vehicle {vehicle_counter['CV']} Route: {route}")
            print(f"  Vehicle {vehicle_counter['CV']} Distance Covered: {distance:.2f}")

def worst_removal(state, rng):
    destroyed = state.copy()
    degree_of_destruction = 0.1
    customers_to_remove = int(data["num_customers"] * degree_of_destruction)
    for customer in sorted(state.routes, key=lambda c: calculate_route_distance(c))[:customers_to_remove]:
        destroyed.unassigned.append(customer)
        route = destroyed.find_route(customer)
        if route:
            route.remove(customer)
    return remove_empty_routes(destroyed)

# def random_insert(state, rng):
#     repaired = state.copy()
#     unassigned_customers = repaired.unassigned[:]
#     rng.shuffle(unassigned_customers)
#     for customer in unassigned_customers:
#         best_route, best_position, best_cost = None, None, float("inf")
#         eligible_routes = [route for route in repaired.routes if can_assign_to_vehicle(route, customer)]
        
#         for route in eligible_routes:
#             for position in range(1, len(route)):
#                 new_route = route[:position] + [customer] + route[position:]
#                 new_cost = calculate_route_distance(new_route)
#                 if new_cost < best_cost:
#                     best_route, best_position, best_cost = route, position, new_cost
#         if best_route is not None:
#             best_route.insert(best_position, customer)
#             repaired.unassigned.remove(customer)
#     return repaired
# Update random_insert to respect max tour lengths
def random_insert(state, rng):
    repaired = state.copy()
    unassigned_customers = repaired.unassigned[:]
    rng.shuffle(unassigned_customers)
    
    for customer in unassigned_customers:
        best_route, best_position, best_cost = None, None, float("inf")
        eligible_routes = [
            (route, vehicle_type, ev_max_tour if vehicle_type == "EV" else cv_max_tour)
            for route, vehicle_type in zip(repaired.routes, repaired.route_types)
            if can_assign_to_vehicle(route, customer)
        ]
        for route, vehicle_type, max_length in eligible_routes:
            for position in range(1, len(route)):
                new_route = route[:position] + [customer] + route[position:]
                new_cost = calculate_route_distance(new_route)
                if new_cost < best_cost and new_cost <= max_length:
                    best_route, best_position, best_cost = route, position, new_cost
        if best_route is not None:
            best_route.insert(best_position, customer)
            repaired.unassigned.remove(customer)
    return repaired

destroy_operators = [random_removal]
repair_operators = [greedy_repair, random_insert]

accept_criteria = [
    HillClimbing(),
    SimulatedAnnealing(start_temperature=1000, end_temperature=1, step=0.9, method="exponential"),
    RecordToRecordTravel(start_threshold=0.05, end_threshold=0.01, step=0.001, method="linear")
]

initialize_data()
init = create_initial_solution()
select = RouletteWheel(scores=[10, 5, 1, 0], decay=0.8, num_destroy=1, num_repair=1)
num_iterations = 1000
best_objective_value = float("inf")
best_solution = None
best_configuration = None

for destroy_op in destroy_operators:
    for repair_op in repair_operators:
        for criterion in accept_criteria:
            print(f"Testing configuration: Destroy = {destroy_op.__name__}, Repair = {repair_op.__name__}, Acceptance = {criterion.__class__.__name__}")
            alns = ALNS()
            alns.add_destroy_operator(destroy_op)
            alns.add_repair_operator(repair_op)
            stop = MaxIterations(num_iterations)
            result = alns.iterate(init, select, criterion, stop)
            solution = result.best_state
            objective_value = solution.objective()
            print(f"Objective value for this configuration: {objective_value:.2f}")
            
            if objective_value < best_objective_value:
                best_objective_value = objective_value
                best_solution = solution
                best_configuration = (destroy_op.__name__, repair_op.__name__, criterion.__class__.__name__)

print("\nBest configuration found:")
print(f"Destroy operator: {best_configuration[0]}")
print(f"Repair operator: {best_configuration[1]}")
print(f"Acceptance criterion: {best_configuration[2]}")
print(f"Best objective value: {best_objective_value:.2f}")
display_solution(best_solution)
plot_routes(best_solution)

