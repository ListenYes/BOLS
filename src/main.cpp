// test on  no_impr_max and no_update_max

#include <cstring>
#include "multi_objective.hpp"

int main(int argc, char** argv) {
    util::setRandom(DEFAULT_RANDOM_SEED);
    TimePoint start_time = util::getTimePoint();


    MultiObjectiveData multi_data;
    multi_data.readDemandFile(argv[1]);
    multi_data.readSampleFile(argv[2]); 
    
    double time_limit = 100;
    time_limit = atoi(argv[3]) * 0.9 - 3;
    

    multi_data.coonstruct_query();
    multi_data.init_allocation();
    long long no_update1 = 0;
    long long no_update2 = 0;
    long long no_update_max1 = atoi(argv[4]);
    long long no_update_max2 = atoi(argv[5]);
    cout << "time: " << time_limit << endl;
    cout << "parameter: " << no_update_max1 << " " << no_update_max2 << endl;
    // long long mode = multi_data.objective_order();
    long long mode = 1;

    for (long long i = 0; i < 10000000; i++){
        // cout << i << endl;
        //multi_data.update_solution();
        //multi_data.do_improve_balance_move();

        if (multi_data.unsat_demand == 0 &&multi_data.unsat_supply == 0){

            multi_data.obj1_available_query_supply = multi_data.calcu_objective1_supply_query();
            // cout << "available: " << multi_data.obj1_available_query_supply << endl;
            if (multi_data.obj1_best_available_query_supply < multi_data.obj1_available_query_supply){     
                // no_impr = 0;           
                multi_data.obj1_best_available_query_supply = multi_data.obj1_available_query_supply;
            }

            if (mode == 1){
                bool update_flag = false;
                if (multi_data.init_solution()){
                    update_flag = true;
                }
                for (int i = 0; i <= 100; i++){
                    if (util::getSeconds(start_time) > time_limit) break;
                    long long query_supply_use = multi_data.obj1_available_query_supply * i / 100;
                    if (multi_data.one_time_update_solution(query_supply_use)){
                        update_flag = true;
                    }
                }
                if (util::getSeconds(start_time) > time_limit) break;
                if (update_flag == false) no_update1++;

                if (mode == 1 && no_update1 >= no_update_max1) {
                    mode = 2;
                    no_update1 = 0;
                } 
            }

            else if (mode == 2){
                bool update_flag = false;
                if (multi_data.init_solution()){
                    update_flag = true;
                }
                for (int i = 0; i <= 100; i++){
                    if (util::getSeconds(start_time) > time_limit) break;
                    long long query_supply_use = multi_data.obj1_available_query_supply * i / 100;
                    if (multi_data.one_time_update_solution(query_supply_use)){
                        update_flag = true;
                    }
                }
                if (util::getSeconds(start_time) > time_limit) break;
                if (update_flag == false) no_update2++;

                if (mode == 2 && no_update2 >= no_update_max2) {
                    mode = 1;
                    no_update2 = 0;
                } 
            }
            else{
                
                multi_data.init_solution();
                for (int i = 0; i <= 100; i++){
                    if (util::getSeconds(start_time) > time_limit) break;
                    long long query_supply_use = multi_data.obj1_available_query_supply * i / 100;
                    multi_data.one_time_update_solution(query_supply_use);
                }
            }
            
            // for (int i = 0; i <= 100; i++){
            //     if (util::getSeconds(start_time) > time_limit) break;
            //     long long query_supply_use = multi_data.obj1_available_query_supply * i / 100;
            //     multi_data.one_time_update_solution(query_supply_use);
            // }
        }
        
        // if (multi_data.unsat_demand == 0 &&multi_data.unsat_supply == 0) multi_data.do_2step_reduce_move();
        if (multi_data.unsat_demand == 0 &&multi_data.unsat_supply == 0){
            if (mode == 1){
                if (multi_data.do_1step_improve_move()){

                }
                else if (multi_data.do_2step_improve_move()){

                }
                else if (multi_data.do_1step_reduce_move_new()){

                }
                else {
                    multi_data.do_2step_reduce_move_new();
                }
            }
            if (mode == 2){
                long long demand = rand() % multi_data.demand_cnt;
                if (multi_data.do_1step_improve_move()){

                }
                else if (multi_data.do_improve_balance_move2(demand)){

                }
                else if (multi_data.do_1step_reduce_move_new_bal2()){
                    
                }
                else{
                    multi_data.do_2step_reduce_move_new_bal2(demand);
                }
            }

            // no_impr++;
        }
            // multi_data.do_2step_reduce_balance_move_new();

        multi_data.do_sat_constraint_move_new();

        // multi_data.update_constraint_weight();
        if (util::getSeconds(start_time) > time_limit) break;
    }

    double best_1_1 = DUMMY_MIN_INT;
    double best_1_2 = DUMMY_MIN_INT;
    double best_1_5 = DUMMY_MIN_INT;
    double best_1_10 = DUMMY_MIN_INT;
    double best_2_1 = DUMMY_MIN_INT;
    double best_5_1 = DUMMY_MIN_INT;
    double best_10_1 = DUMMY_MIN_INT;

    cout << "best size: " << multi_data.best_obj_value_vec.size() << endl;
    for (int i = 0; i < multi_data.best_obj_value_vec.size(); i++){
        cout << multi_data.best_obj_value_vec[i].obj1 << " " << multi_data.best_obj_value_vec[i].obj2 << " " <<  multi_data.best_obj_value_vec[i].delete_flag << endl;
        double r1_1 = multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2;
        double r1_2 = multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2 * 2;
        double r1_5 = multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2 * 5;
        double r1_10 = multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2 * 10;
        double r2_1 = 2 * multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2;
        double r5_1 = 5 * multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2;
        double r10_1 = 10 * multi_data.best_obj_value_vec[i].obj1 - multi_data.best_obj_value_vec[i].obj2;
        if (r1_1 > best_1_1) best_1_1 = r1_1;
        if (r1_2 > best_1_2) best_1_2 = r1_2;
        if (r1_5 > best_1_5) best_1_5 = r1_5;
        if (r1_10 > best_1_10) best_1_10 = r1_10;
        if (r2_1 > best_2_1) best_2_1 = r2_1;
        if (r5_1 > best_5_1) best_5_1 = r5_1;
        if (r10_1 > best_10_1) best_10_1 = r10_1; 
    }
    cout << "1:1 best: " << best_1_1 << endl;
    cout << "1:2 best: " << best_1_2 << endl;
    cout << "1:5 best: " << best_1_5 << endl;
    cout << "1:10 best: " << best_1_10 << endl;
    cout << "2:1 best: " << best_2_1 << endl;
    cout << "5:1 best: " << best_5_1 << endl;
    cout << "10:1 best: " << best_10_1 << endl;
    
    cout << "time: " << util::getSeconds(start_time) << " limit " << time_limit << endl;
    // multi_data.hyper_volume();

    for (auto supply : multi_data.unsat_supply_vec){
        cout << supply << " " << multi_data.supply_value[supply] << " " << multi_data.supply_remain[supply] << endl;
    }
    cout << "unsat supply: " << multi_data.unsat_supply << endl;

    return 0;   

}