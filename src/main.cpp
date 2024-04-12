
#include <cstring>
#include "multi_objective.hpp"

int main(int argc, char** argv) {
    util::setRandom(DEFAULT_RANDOM_SEED);
    TimePoint start_time = util::getTimePoint();

    double time_limit = 100;
    cout << "time limit: " << time_limit << endl;
    time_limit = atoi(argv[3]) * 0.9 - 3;

    long long no_update1 = 0;
    long long no_update2 = 0;
    long long no_update_max1 = 10;
    long long no_update_max2 = 10;
    if (argc >= 6) {
        no_update_max1 = atoi(argv[4]);
        no_update_max2 = atoi(argv[5]);
    }

    
    cout << "parameter: " << no_update_max1 << " " << no_update_max2 << endl;


    MultiObjectiveData multi_data;
    multi_data.readDemandFile(argv[1]);
    multi_data.readSampleFile(argv[2]); 
    

    

    multi_data.coonstruct_query();
    multi_data.init_allocation();

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



    if (multi_data.unsat_supply <= 0 && multi_data.unsat_demand <= 0) 
    {
        cout << "find feasible solution " << endl;
    }
    else 
    {
        cout << "not find feasible solution " << endl; 
        cout << "unsat supply: " << multi_data.unsat_supply << endl;

        for (auto supply : multi_data.unsat_supply_vec)
        {
            cout << supply << " " << multi_data.supply_value[supply] << " " << multi_data.supply_remain[supply] << endl;
        }
    }

    cout << "best solution set size: " << multi_data.best_obj_value_vec.size() << endl;
    cout << "*****************************" << endl;

    for (int i = 0; i < multi_data.best_obj_value_vec.size(); i++)
    {
        cout << - multi_data.best_obj_value_vec[i].obj1 << " " << multi_data.best_obj_value_vec[i].obj2  << endl;

    }
    
    // cout << "time: " << util::getSeconds(start_time) << " limit " << time_limit << endl;
    // multi_data.hyper_volume();

    return 0;   

}
