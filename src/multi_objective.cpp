#include "multi_objective.hpp"

void MultiObjectiveData::readDemandFile(string filePath) {
    std::ifstream fileStream(filePath);
    string line;
    assert(demand_cnt == 0);

    char splitChar = '-';
    
    while (getline(fileStream, line)) {
        if (line[0] == 'a') continue;

        if (splitChar == '-') {
            if (line.find("`") != string::npos) splitChar = '`';
            if (line.find(";") != string::npos) splitChar = ';';
            if (line.find(",") != string::npos) splitChar = ',';
            assert(splitChar != '-');
        }

        vector<string> demandData = util::splitStr(line, splitChar);
        assert(demandData.size() == 2);
        string demandID = demandData.at(0);
        Int demandV = std::stoi(demandData.at(1));

        if (!haveDemand(demandID)) {
            demandID2index.insert(pair<string, long long>(demandID,demand_cnt));
            demand_value.push_back(demandV / DEMAND_DIVISOR);
            demand_remain.push_back(demandV / DEMAND_DIVISOR);
            total_demand_supply_value.push_back(0);
            map_demand2supply.push_back(vector<long long>());
            allocat_position_in_supply.push_back(vector<long long>());

            //index2demandID, debug
            index2demandID.insert(pair<long long, string>(demand_cnt,demandID));

            demand_sum += demandV;
        }
        else util::showError("Duplicate demandID " + demandData.at(0));

        demand_cnt++;

        assert(demand_value.size() == demand_cnt);
    }
    fileStream.close();
}

void MultiObjectiveData::readSampleFile(string filePath) {
    std::ifstream fileStream(filePath);
    string line;
    assert(supply_cnt == 0);
    assert(demand_cnt > 0);

    char splitChar = '-';

    while (getline(fileStream, line)) {
        if (line[0] == 'a') continue;

        // if (genRandom() % 100 > READ_RATE) { // READ_RATE = 10  only read 10% original
        //     continue;
        // }

        if (splitChar == '-') {
            if (line.find("`") != string::npos) splitChar = '`';
            if (line.find(";") != string::npos) splitChar = ';';
            assert(splitChar != '-');
        }

        vector<string> sampleData = util::splitStr(line, splitChar);
        //cout << sampleData[0] << " " << sampleData[1] << " " << sampleData[2] << " " << sampleData[3] << endl;
        assert(sampleData.size() == 4);
        string usrID = sampleData.at(0), supplyID = sampleData.at(1);
        
        pair<string, string> usr_supply_pair = std::make_pair(usrID,supplyID);
        //supplyID2index.insert(std::make_pair(usr_supply_pair,supply_cnt));

        // if (util::isFound(usr_supply_pair, supplyID2index.at(usrIndex))) {
        //     util::showWarning("Duplicate UsrID: " + sampleData.at(0) + " and supplyID: " + sampleData.at(1)); 
        // } 

        long long supply_index = supply_cnt;
        supplyID2index[usr_supply_pair] = supply_cnt;

        // index2supplyID, debug
        index2supplyID[supply_cnt] = usrID;

        map_supply2demand.push_back(vector<long long>());
        assign_supply2demand.push_back(vector<allocateVar>());
        obj2_const_coef.push_back(vector<double>());
        //allocat_supply2demand.push_back(vector<long long>());

        // // <usrIndex, supplyIndex> -> _tableCnt
        // _usrSupply.at(usrIndex).push_back(supplyIndex);
        // _usrDemand.at(usrIndex).push_back(_tableCnt);

        assert(supply_value.size() == supply_cnt);
        Int supplyLimit = std::stoll(sampleData.at(2));
        supply_value.push_back(supplyLimit);
        supply_remain.push_back(supplyLimit);

        supply_sum += supplyLimit;

        vector<string> demandList = util::splitStr(sampleData.at(3), ',');
        vector<Int>    curDemand;

        curDemand.push_back(supplyLimit);
        Set<Int> visitedDemandIndex;
        for (string demandID : demandList) {
            if (!haveDemand(demandID)) {
                // util::showWarning("DemandID: " + demandID + " haven't appear before !!!");
                continue;
            }
            Int demand_index = demandID2index[demandID];
            if (visitedDemandIndex.count(demand_index) > 0) continue;
            visitedDemandIndex.insert(demand_index);

            curDemand.push_back(demand_index);

            //construct map
            map_supply2demand[supply_index].push_back(demand_index);
            map_demand2supply[demand_index].push_back(supply_index);
            obj2_const_coef[supply_index].push_back(0);

            //assign values are ordered the same as <map_supply2demand>
            //if you have a supply and want to find all assign vars, it is easy
            //but if you have a Demand Index, it is hard to find the var values connected to it
            //so I record all the position of vars in <map_supply2demand> and order them the same as <map_demand2supply>
            //For example, you have demand index (D_i) ,you can find supply index (S_i1, S_i2) connect to it
            //position1 = allocat_position_in_supply[D_i][S_i1]
            //assign_supply2demand[S_i1][position1] is the var value
            long long position = map_supply2demand[supply_index].size() - 1;
            allocat_position_in_supply[demand_index].push_back(position);

            allocateVar var(supply_index,demand_index,0);
            assign_supply2demand[supply_index].push_back(var);

            total_demand_supply_value[demand_index] += supplyLimit;
        }

        supply_cnt++;
    }
    unsat_supply_vec.init(supply_cnt + 1);
    unsat_demand_vec.init(demand_cnt + 1);
    query_supply_not_full.init(supply_cnt + 1);
    unsat_demand = 0;
    unsat_supply = 0;

    weight_obj1 = 1;
    obj1_total_query_supply = 0;
    weight_demand.resize(demand_cnt + 1, 1);
    weight_supply.resize(supply_cnt + 1, 1);
    obj1_supply_flag.resize(supply_cnt + 1, 0);
    show_data();

    for (long long demand = 0; demand < demand_cnt; demand++) {
        if (demand_value[demand] > 0){
            unsat_demand_vec.push_back(demand);
            unsat_demand++;
        }
    }
    best_unsat_num = unsat_supply + unsat_demand;

    // cout << "sum" << demand_sum << " " << supply_sum << " " << demand_sum / supply_sum << endl;

}

void MultiObjectiveData::show_data(){
    cout << "demand cnt : " << demand_cnt << endl;
    cout << "supply cnt : " << supply_cnt << endl;

    for(long long demand = 0; demand < demand_cnt; demand++){
        // cout << demand << " " << demand_value[demand] << "  finish  " << endl;
        for (long long j = 0; j < map_demand2supply[demand].size(); j++){
            long long supply = map_demand2supply[demand][j];
            long long position = getAlloPosition(demand,j);
            allocateVar var = assign_supply2demand[supply][position];
            //cout << demand << "  " << var.demand_index << endl;
            assert(var.demand_index == demand);
            assert(var.suppy_index == supply);
            assert(var.allocate_value == 0);
        }
    }
}

struct balance_coefficient
{
    double val;
    long long demand_index;
    long long supply_index;
    long long position;
};

bool cmp_balance_coef(balance_coefficient b1, balance_coefficient b2){
    return b1.val > b2.val;
}

// using the last demand to construct the query, 
// query = the last demand
void MultiObjectiveData::coonstruct_query(){
    long long demand_index = demand_cnt - 1;
    for (auto supply : map_demand2supply[demand_index]){
        obj1_supply_flag[supply] = 1;
        obj1_query_supply.push_back(supply);
        obj1_total_query_supply += supply_value[supply];
    }


    obj1_available_query_supply = obj1_total_query_supply;
    obj1_best_available_query_supply = 0;
}


void MultiObjectiveData::init_allocation(){
    
    for(long long demand_index = 0; demand_index < demand_cnt; demand_index++){
        vector<balance_coefficient> balance_vec;

        for(long long i = 0; i < map_demand2supply[demand_index].size(); i++){

            long long supply_index1 = map_demand2supply[demand_index][i];
            long long position      = allocat_position_in_supply[demand_index][i];
            long long supply_val    = supply_value[supply_index1];
            long long demand_val    = demand_value[demand_index];
            long long total_demand_supply_val = get_total_demand_supply_value(demand_index);
            
            balance_coefficient balance_coef;
            balance_coef.val = (double)supply_val / total_demand_supply_val * demand_val;
            balance_coef.demand_index = demand_index;
            balance_coef.supply_index = supply_index1;
            balance_coef.position = position;
            balance_vec.push_back(balance_coef);

            obj2_const_coef[supply_index1][position] = balance_coef.val;
            //cout << "supply: " << supply_index1 << " " << demand_index << endl;
            //cout << supply_val << " " << total_demand_supply_val << " " << balance_coef.val << endl;
        }

        std::sort(balance_vec.begin(),balance_vec.end(),cmp_balance_coef);
        // cout << "-------------balance vec size" << balance_vec.size() << endl;

        long long i = 0;
        for (balance_coefficient balance_coef : balance_vec){

            if (demand_remain[demand_index] <= 0) break;
            
            else {
                long long bal_val = round(balance_coef.val);
                long long val = std::max(bal_val,(long long)1);
                val = std::min({val, demand_remain[balance_coef.demand_index], supply_remain[balance_coef.supply_index]});
                if (val != assign_supply2demand[balance_coef.supply_index][balance_coef.position].allocate_value){
                    change_assignment(balance_coef.supply_index, balance_coef.position, balance_coef.demand_index, val);
                }
            }
        }
    }


    // init supply which are queried
    long long query_supply_sum = 0;
    for (auto supply : obj1_query_supply){
        query_supply_sum += supply_value[supply];
    }
    for (auto supply : obj1_query_supply){
        double coef = (double) supply_value[supply] / query_supply_sum;
        query_const_coef.push_back(coef);
    }
    query_const_coef.resize(0,obj1_query_supply.size() + 1);

    //for (long long demand = 0; demand < demand_cnt; demand++);
    cout << "*****************************" << endl;
    // cout << "Init assignment obj2 value:  " << calcu_objective2_value() << endl;
    // cout << "unsat num : " << unsat_demand + unsat_supply << endl;
    // cout << "unsat demand: " << unsat_demand << endl;
    // for (auto demand : unsat_demand_vec){
    //     cout << index2demandID[demand] << " " << demand_value[demand] << " " << demand_remain[demand] << endl;
    // }
    // cout << "unsat supply: " << unsat_supply << endl;

    while (unsat_demand > 0)
    {
        do_sat_demand_move_2();
    }
    // cout << "unsat supply: " << unsat_supply << endl;
    // cout << "unsat demand: " << unsat_demand << endl;
    
}

bool MultiObjectiveData::init_solution(){

    // 0 solution
    query_use_total = 0;
    query_use.clear();
    for (auto supply : obj1_query_supply){
        query_use.push_back(0);
    }
    update_solution2(0);

    // full solution
    query_use_total = 0;
    query_use.clear();
    for (auto supply : obj1_query_supply){
        long long supply_provide = supply_remain[supply];
        query_use.push_back(supply_provide);
        query_use_total += supply_provide;
    }

    double    obj2 = calcu_objective2_value();
    for (int i = 0; i < obj1_query_supply.size(); i++){
        obj2 += abs((double) query_const_coef[i] * query_use_total - query_use[i]);
    }

    bool flag = update_solution2(0);

    query_use_total = 0;
    query_use.clear();

    if (flag == true) return true;
    else return false;
    // return obj2;
}

bool MultiObjectiveData::check_demand_sat_state(long long demand){
    if (demand_remain[demand] > 0) {
        return false;
    }
    else return true;
}

bool MultiObjectiveData::check_supply_sat_state(long long supply){
    if (supply_remain[supply] <0 || supply_remain[supply] > supply_value[supply]) {
        return false;
    }
    else return true;
}

bool MultiObjectiveData::change_demand_sat_state(bool old_state, bool new_state, long long demand){
    assert(old_state != new_state);
    if (old_state == new_state) return false;
    if (old_state == false && new_state == true){
        unsat_demand_vec.remove(demand);
        unsat_demand--;
    }
    else if (old_state == true && new_state == false){
        unsat_demand_vec.push_back(demand);
        unsat_demand++;
    }
    return true;
}

bool MultiObjectiveData::change_supply_sat_state(bool old_state, bool new_state, long long supply){

    assert(old_state != new_state);
    if (old_state == new_state) return false;
    if (old_state == false && new_state == true){
        //cout << "f->t" << endl;
        unsat_supply_vec.remove(supply);
        unsat_supply--;
    }
    else if (old_state == true && new_state == false){
        //cout << "t->f" << endl;
        unsat_supply_vec.push_back(supply);
        unsat_supply++;
    }
    return true;
}

bool MultiObjectiveData::change_assignment(long long supply, long long supply_position, long long demand, long long new_val){
    allocateVar &assign = assign_supply2demand[supply][supply_position];
    long long old_value = assign.allocate_value;
    long long value_change = new_val -old_value;
    
    bool demand_sat = check_demand_sat_state(demand);
    bool supply_sat = check_supply_sat_state(supply);

    //cout << supply << "  " << demand << "  " << old_value << "  " << new_val << "  " << value_change << endl;
    demand_remain[demand] -= value_change;
    supply_remain[supply] -= value_change;
    assign.allocate_value = new_val;

    bool new_demand_state = check_demand_sat_state(demand);
    bool new_supply_state = check_supply_sat_state(supply);
    
    if (demand_sat != new_demand_state){
        //cout << unsat_demand << endl;
        //cout << demand_sat << "   " << new_demand_state << endl;
        change_demand_sat_state(demand_sat, new_demand_state, demand);
    }
    if (supply_sat != new_supply_state){
        //cout << supply_sat << "   " << new_supply_state << endl;
        change_supply_sat_state(supply_sat, new_supply_state, supply);
    }

    if (obj1_supply_flag[supply] == 1){
        obj1_available_query_supply += value_change;
        if (supply_remain[supply] != supply_value[supply]){
            query_supply_not_full.push_back(supply);
        }
        if (supply_remain[supply] == supply_value[supply]){
            if (query_supply_not_full.exist(supply)) query_supply_not_full.remove(supply);
        }
    }
    return true;
}

double MultiObjectiveData::calcu_objective2_value(){
    double total_obj2_value = 0.0;

    for (long long supply = 0; supply < supply_cnt; supply++){
        for (long long position = 0; position < map_supply2demand[supply].size(); position++){
            long long demand = map_supply2demand[supply][position];

            allocateVar var = assign_supply2demand[supply][position];
            double var_value = var.allocate_value;
            double var_objective_value = obj2_const_coef[supply][position];
            double diff = std::abs(var_value - var_objective_value);
            //cout << var_value << "  " << var_objective_value << "  " << diff << "  " << std::abs(diff) << endl;
            total_obj2_value += diff;
        }
    }

    return total_obj2_value;
}

long long MultiObjectiveData::calcu_objective1_supply_query(){
    long long total_remain = 0;
    for (long long supply : obj1_query_supply){
        total_remain += supply_remain[supply];
    }
    return total_remain;
}

// supply is exceed, and we need to decrease the use of supply
void MultiObjectiveData::do_sat_supply_move(long long supply){
    for (long long position = 0; position < map_supply2demand[supply].size(); position++){
        allocateVar var = assign_supply2demand[supply][position];
        long long demand = map_supply2demand[supply][position];
        assert(supply_remain[supply] < 0);
        long long supply_exceed = - supply_remain[supply];
        

        if (demand_remain[demand] < 0){
            long long diff = std::min({- demand_remain[demand], var.allocate_value, supply_exceed});
            if (diff > 0){
                change_assignment(supply, position, demand, var.allocate_value - diff);
            }
        }
        else {
            int BMS_temp = BMS;
            if (BMS_temp > map_demand2supply[demand].size()) BMS_temp = map_demand2supply[demand].size();
            for (int i = 0; i < BMS_temp; i++){
                long long index    = rand() % map_demand2supply[demand].size();
                long long supply_2 = map_demand2supply[demand][index];
                allocateVar var = assign_supply2demand[demand][index];
                if (supply_remain[supply_2] > 0){
                    long long trans_val = std::min({supply_exceed, supply_remain[supply_2], var.allocate_value});
                }
            }
        }
    }
}

void MultiObjectiveData::do_sat_demand_move_2(){
    for (auto demand : unsat_demand_vec){
        for (long long position_in_demand = 0; position_in_demand < map_demand2supply[demand].size(); position_in_demand++){
            long long supply = map_demand2supply[demand][position_in_demand];
            long long position_in_supply = allocat_position_in_supply[demand][position_in_demand];
            allocateVar var = assign_supply2demand[supply][position_in_supply];
            // cout << "size" << unsat_demand_vec.size() << "  " << unsat_demand << endl;
            // cout << "demand: " << demand << " remain:  " << demand_remain[demand] << endl;
            assert(demand_remain[demand] > 0);
            long long demand_exceed = demand_remain[demand];

            // long long diff = std::min( supply_remain[supply], demand_exceed);
            long long diff = rand() % (demand_exceed + 1);
            if (diff > 0){
                change_assignment(supply, position_in_supply, demand, var.allocate_value + diff);
                if (check_demand_sat_state(demand))break;
            }
            
        }
    }
}

void MultiObjectiveData::do_sat_constraint_move(){

    assert(unsat_demand == 0);

    if (unsat_supply == 0) return;
    
    //cout << "unsat supply" << endl;
    //for (auto supply : unsat_supply_vec) cout << supply << "  ";
    //cout << endl;

    for (auto supply : unsat_supply_vec){
        for (long long position = 0; position < map_supply2demand[supply].size(); position++){
            allocateVar var = assign_supply2demand[supply][position];
            long long demand = map_supply2demand[supply][position];
            // cout << "supply: " << supply << " remain: " << supply_remain[supply] << "  value: " << supply_value[supply] << endl;
            assert(supply_remain[supply] < 0);
            long long supply_exceed = - supply_remain[supply];
            

            if (demand_remain[demand] < 0){
                long long diff = std::min( {- demand_remain[demand], var.allocate_value, supply_exceed});
                if (diff > 0){
                    change_assignment(supply, position, demand, var.allocate_value - diff);
                    // cout << "supply condition: " << check_supply_sat_state(supply) << endl;
                    if (check_supply_sat_state(supply)) break;
                }
            }

            else {
                int BMS_temp = BMS;
                if (BMS_temp > map_demand2supply[demand].size()) BMS_temp = map_demand2supply[demand].size();
                for (int i = 0; i < BMS_temp; i++){
                    long long posi_in_demand = rand() % map_demand2supply[demand].size();
                    long long supply_2 = map_demand2supply[demand][posi_in_demand];
                    long long posi_in_supply = allocat_position_in_supply[demand][posi_in_demand];
                    allocateVar var_2 = assign_supply2demand[supply_2][posi_in_supply];
                    if (supply_remain[supply_2] > 0){
                        long long trans_val = std::min({supply_exceed, supply_remain[supply_2], var.allocate_value});
                        if (trans_val > 0){
                            change_assignment(supply, position, demand, var.allocate_value - trans_val);
                            change_assignment(supply_2, posi_in_supply, demand, var_2.allocate_value + trans_val);
                            if (check_supply_sat_state(supply)) break;
                        }
                    }
                }
                if (check_supply_sat_state(supply)) break;
            }
        }
    }

    //cout << endl;
    //for (auto demand : unsat_demand_vec) cout << demand << "  ";
    //cout << endl;

    for (auto demand : unsat_demand_vec){
        for (long long position_in_demand = 0; position_in_demand < map_demand2supply[demand].size(); position_in_demand++){
            long long supply = map_demand2supply[demand][position_in_demand];
            long long position_in_supply = allocat_position_in_supply[demand][position_in_demand];
            allocateVar var = assign_supply2demand[supply][position_in_supply];
            // cout << "size" << unsat_demand_vec.size() << "  " << unsat_demand << endl;
            // cout << "demand: " << demand << " remain:  " << demand_remain[demand] << endl;
            assert(demand_remain[demand] > 0);
            long long demand_exceed = demand_remain[demand];

            if (supply_remain[supply] > 0){
                long long diff = std::min( supply_remain[supply], demand_exceed);
                if (diff > 0){
                    change_assignment(supply, position_in_supply, demand, var.allocate_value + diff);
                    if (check_demand_sat_state(demand))break;
                }
            }

            else {
                int BMS_temp = BMS;
                if (BMS_temp > map_supply2demand[supply].size()) BMS_temp = map_supply2demand[supply].size();
                for (int i = 0; i < BMS_temp; i++){
                    long long posi2_in_supply = rand() % map_supply2demand[supply].size();
                    long long demand_2 = map_supply2demand[supply][posi2_in_supply];
                    allocateVar var_2 = assign_supply2demand[supply][posi2_in_supply];
                    if (demand_remain[demand_2] < 0){
                        long long trans_val = std::min({demand_exceed, -demand_remain[demand_2], var_2.allocate_value});
                        if (trans_val > 0){
                            change_assignment(supply, position_in_supply, demand, var.allocate_value + trans_val);
                            change_assignment(supply, posi2_in_supply, demand_2, var_2.allocate_value - trans_val);
                            if (check_demand_sat_state(demand))break;
                        }
                    }
                }
                if (check_demand_sat_state(demand))break;
            }
        }
    }
    return ;
}

void MultiObjectiveData::do_sat_constraint_move_new(){

    assert(unsat_demand == 0);
    if (unsat_supply == 0) return;

    long long index = rand() % unsat_supply_vec.size();
    long long supply = unsat_supply_vec[index];

    // cout << "supply: " << supply << endl << "demand: " ;
    // for (auto demand : map_supply2demand[supply]) cout << demand << " ";
    // cout << endl;

    for (long long position = 0; position < map_supply2demand[supply].size(); position++){
        allocateVar var = assign_supply2demand[supply][position];
        long long demand = map_supply2demand[supply][position];
        // cout << "supply: " << supply << " remain: " << supply_remain[supply] << "  value: " << supply_value[supply] << endl;
        assert(supply_remain[supply] < 0);
        long long supply_exceed = - supply_remain[supply];
        

        if (demand_remain[demand] < 0){
            long long diff = std::min( {- demand_remain[demand], var.allocate_value, supply_exceed});
            if (diff > 0){
                change_assignment(supply, position, demand, var.allocate_value - diff);
                // cout << "supply condition: " << check_supply_sat_state(supply) << endl;
                if (check_supply_sat_state(supply)) break;
            }
        }

        else {

            allocateVar best_var1;
            allocateVar best_var2;
            double best_score = DUMMY_MIN_INT;
            long long pos1 = 0;
            long long pos2 = 0;
            long long best_trans_val = 0;

            int BMS_temp = BMS;
            if (BMS_temp > map_demand2supply[demand].size()) BMS_temp = map_demand2supply[demand].size();
            for (int i = 0; i < BMS_temp; i++){
                long long posi_in_demand = rand() % map_demand2supply[demand].size();
                long long supply_2 = map_demand2supply[demand][posi_in_demand];
                long long posi_in_supply = allocat_position_in_supply[demand][posi_in_demand];
                allocateVar var_2 = assign_supply2demand[supply_2][posi_in_supply];
                
                long long trans_val = rand() % (var.allocate_value + 1);
                if (trans_val > supply_exceed) trans_val = supply_exceed;
                // long long trans_val = std::min({supply_exceed, supply_remain[supply_2], var.allocate_value});

                // if (trans_val > 0){
                //     change_assignment(supply, position, demand, var.allocate_value - trans_val);
                //     change_assignment(supply_2, posi_in_supply, demand, var_2.allocate_value + trans_val);
                //     if (check_supply_sat_state(supply)) break;
                // }
                double cur_score = calcu_2step_operator_score_new(var, var_2, trans_val);
                //cout << "try: supply1: " << var.suppy_index << " supply2: " << var_2.suppy_index << " demand: " << var.demand_index << " " << var_2.demand_index  << " trans_val: " << trans_val << " " << var.allocate_value << endl;

                if (cur_score > best_score){
                    best_score      = cur_score;
                    best_trans_val  = trans_val;
                    best_var1 = var;
                    best_var2 = var_2;
                    pos1 = position;
                    pos2 = posi_in_supply;
                }
            }
            if (best_trans_val > 0){
                long long supply_1 = best_var1.suppy_index;
                long long demand_1 = best_var1.demand_index;
                long long supply_2 = best_var2.suppy_index;
                long long demand_2 = best_var2.demand_index;
                // cout << "supply1: " << supply_1 << " supply2: " << supply_2 << " demand: " << demand_1 << " " << demand_2  << " trans_val: " << best_trans_val << endl;

                change_assignment(supply_1, pos1, demand_1, best_var1.allocate_value - best_trans_val);
                change_assignment(supply_2, pos2, demand_2, best_var2.allocate_value + best_trans_val);

                // cout << "unsat num: " << unsat_demand << " " << unsat_supply << endl;
                // for (auto s : unsat_supply_vec) cout << s << " " << supply_remain[s] << endl;
                
            }
            if (check_supply_sat_state(supply)) break;
        }
    }
    
    return ;
}


struct Supply_choice
{
    long long supply2;
    long long supply2_pos;
    long long demand;
    long long val2;
    double coef2;
};

bool supply_choice_cmp(Supply_choice s1, Supply_choice s2){
    return (s1.coef2 - s1.val2) > (s2.coef2 - s2.val2);
}


// void MultiObjectiveData::do_sat_constraint_move_new2(){

//     assert(unsat_demand == 0);
//     if (unsat_supply == 0) return;

//     long long index = rand() % unsat_supply_vec.size();
//     long long supply1 = unsat_supply_vec[index];

//     vector<Supply_choice> choice_vec;

//     for (long long demand_pos = 0; demand_pos < map_supply2demand[supply1].size(); demand_pos++){

//         allocateVar var1 = assign_supply2demand[supply1][demand_pos];
//         long long demand = map_supply2demand[supply1][demand_pos];
//         long long supply_exceed = - supply_remain[supply1];

//         assert(supply_remain[supply1] < 0);

//         for (long long pos = 0; pos < map_demand2supply[demand].size(); pos++){
//             long long supply2 = map_demand2supply[demand][pos];
//             long long supply2_pos = allocat_position_in_supply[demand][pos];
//             allocateVar var2 = assign_supply2demand[supply2][supply2_pos];

//             if (supply_remain[supply2] > 0){
//                 Supply_choice supply_choice;
//                 supply_choice.supply2 = supply2;
//                 supply_choice.demand = demand;
//                 supply_choice.supply2_pos = supply2_pos;
//                 supply_choice.coef2 = obj2_const_coef[supply2][supply2_pos];
//                 supply_choice.val2 = var2.allocate_value;
//                 choice_vec.push_back(supply_choice);
//             }
//         }
//     }

//     std::sort(choice_vec.begin(), choice_vec.end(), supply_choice_cmp);

//     for (auto supply_choice : choice_vec){
//         long long val = round(supply_choice.coef2 - supply_choice.val2);
//         //val = std::min(val, supply_remain[supply_choice.supply2], )
//     }
    
//     return ;
// }

void MultiObjectiveData::do_improve_balance_move(){

    for(long long demand_index = 0; demand_index < demand_cnt; demand_index++){
        long long available_supply_all = 0;
        vector<balance_coefficient> balance_vec;

        for(long long i = 0; i < map_demand2supply[demand_index].size(); i++){

            long long supply_index1 = map_demand2supply[demand_index][i];
            long long position      = allocat_position_in_supply[demand_index][i];
            double coef = obj2_const_coef[supply_index1][position];

            if (obj1_supply_flag[supply_index1] == 1) continue;
            
            balance_coefficient balance_coef;
            balance_coef.val = (double)coef;
            balance_coef.demand_index = demand_index;
            balance_coef.supply_index = supply_index1;
            balance_coef.position = position;
            balance_vec.push_back(balance_coef);

            available_supply_all += (supply_remain[supply_index1] + assign_supply2demand[supply_index1][position].allocate_value);
        }

        std::sort(balance_vec.begin(),balance_vec.end(),cmp_balance_coef);
        long long demand_use = 0;

        for (balance_coefficient balance_coef : balance_vec){
            change_assignment(balance_coef.supply_index, balance_coef.position, balance_coef.demand_index, 0);
        }

        for (balance_coefficient balance_coef : balance_vec){
            if (demand_remain[demand_index] <= 0) break;
            
            else {
                long long bal_val = round(balance_coef.val);
                long long val = std::max(bal_val,(long long)1);
                val = std::min({val, demand_remain[balance_coef.demand_index], supply_remain[balance_coef.supply_index]});
                if (val != assign_supply2demand[balance_coef.supply_index][balance_coef.position].allocate_value){
                    change_assignment(balance_coef.supply_index, balance_coef.position, balance_coef.demand_index, val);
                }
                demand_use += val;
            }
        }
    }
}

bool MultiObjectiveData::do_improve_balance_move2(long long demand){
    // long long demand = rand() % demand_cnt;
    vector<long long> val_need_increase;    //restore pos
    vector<long long> val_need_decrease;    //restore pos

    bool find_flag = false;

    for (long long pos = 0; pos < map_demand2supply[demand].size(); pos++){
        long long supply = map_demand2supply[demand][pos];
        long long supply_pos = allocat_position_in_supply[demand][pos];
        if (obj1_supply_flag[supply] == 1) continue;
        if (obj2_const_coef[supply][supply_pos] - assign_supply2demand[supply][supply_pos].allocate_value > 0 && supply_remain[supply] > 0){
            val_need_increase.push_back(pos);
        }
        if (obj2_const_coef[supply][supply_pos] - assign_supply2demand[supply][supply_pos].allocate_value < -1){
            val_need_decrease.push_back(pos);
        }
    }

    while (!val_need_decrease.empty() && !val_need_increase.empty())
    {
        long long pos1_demand = val_need_decrease.back();   //1 - decrease
        long long pos2_demand = val_need_increase.back();   //2 - increase
        val_need_decrease.pop_back();
        val_need_increase.pop_back();
        long long supply1 = map_demand2supply[demand][pos1_demand];
        long long supply2 = map_demand2supply[demand][pos2_demand];
        long long pos1_supply = allocat_position_in_supply[demand][pos1_demand];
        long long pos2_supply = allocat_position_in_supply[demand][pos2_demand];

        long long val1 = assign_supply2demand[supply1][pos1_supply].allocate_value - obj2_const_coef[supply1][pos1_supply];
        long long val2 = supply_remain[supply2];
        long long trans_val = std::min(val1, val2);

        change_assignment(supply1, pos1_supply, demand, assign_supply2demand[supply1][pos1_supply].allocate_value - trans_val);
        change_assignment(supply2, pos2_supply, demand, assign_supply2demand[supply2][pos2_supply].allocate_value + trans_val);
        find_flag = true;

        if (obj2_const_coef[supply1][pos1_supply] - assign_supply2demand[supply1][pos1_supply].allocate_value < -1){
            val_need_decrease.push_back(pos1_demand);
        }
        if (obj2_const_coef[supply2][pos2_supply] - assign_supply2demand[supply2][pos2_supply].allocate_value > 0 && supply_remain[supply2] > 0){
            val_need_increase.push_back(pos2_demand);
        }

    }
    if (find_flag == true) return true;
    else return false;
    
}

long long MultiObjectiveData::calcu_operator_score(allocateVar var, long long var_val_new){

    long long sat_num_change = 0;

    long long var_old_val = var.allocate_value;
    long long supply = var.suppy_index;
    long long demand = var.demand_index;
    //cout << supply << "  " << demand << "  " << var_old_val << endl;
    long long val_change = var_val_new - var_old_val;
    bool cur_supply_state = check_supply_sat_state(supply);
    bool cur_demand_state = check_demand_sat_state(demand);
    long long demand_remain_new = demand_remain[demand] - val_change;
    long long supply_remain_new = supply_remain[supply] - val_change;


    long long score = 0;
    if (cur_demand_state && demand_remain_new > 0){ //sat -> unsat
        score -= weight_demand[demand];
    } 
    else if (!cur_demand_state && demand_remain_new <= 0){ //unsat-> sat
        score += weight_demand[demand];
    }

    if (cur_supply_state && (supply_remain_new <0 || supply_remain_new > supply_value[supply])){//sat->unsat
        score -= weight_supply[supply];
    }
    else if(!cur_supply_state && !(supply_remain[supply] <0 || supply_remain[supply] > supply_value[supply])){//unsat -> sat
        score += weight_supply[supply];
    }

    if (obj1_supply_flag[supply] == 1){
        score = score - val_change * weight_obj1;
    }
    cout << "score cal: " << var.allocate_value << "  " << var_val_new << " " << score << endl;
    return score;
}

long long MultiObjectiveData::calcu_2step_operator_score(allocateVar &var1, allocateVar &var2, long long trans_val){
    long long score = 0;
    long long var1_supply = var1.suppy_index;
    long long var2_supply = var2.suppy_index;
    bool var1_supply_state = check_supply_sat_state(var1_supply);
    bool var2_supply_state = check_supply_sat_state(var2_supply);
    long long var1_val_new = var1.allocate_value - trans_val;
    long long var2_val_new = var2.allocate_value + trans_val;

    if (var1_supply_state == false){    //false -> true
        if (var1_val_new >= 0 && var1_val_new <= supply_value[var1_supply]){
            score += weight_supply[var1_supply];
        }
    }
    else if (var1_supply_state == true){     //true -> false
        if (var1_val_new < 0 || var1_val_new > supply_value[var1_supply]){
            score -= weight_supply[var1_supply];
        }
    }

    if (var2_supply_state == false){        //false -> true
        if (var2_val_new >= 0 && var2_val_new <= supply_value[var2_supply]){
            score += weight_supply[var2_supply];
        }
    }
    else if (var2_supply_state == true){    //true -> false
        if (var2_val_new < 0 || var2_val_new > supply_value[var2_supply]){
            score -= weight_supply[var2_supply];
        }
    }

    if (obj1_supply_flag[var1_supply] == 1 && obj1_supply_flag[var2_supply] == 0){
        score += weight_obj1 * trans_val;
    }
    if (obj1_supply_flag[var1_supply] == 0 && obj1_supply_flag[var2_supply] == 1){
        score -= weight_obj1 * trans_val;
    }

    return score;
}

long long MultiObjectiveData::calcu_2step_operator_score_new(allocateVar &var1, allocateVar &var2, long long trans_val){
    long long score = 0;
    long long var1_supply = var1.suppy_index;
    long long var2_supply = var2.suppy_index;
    bool var1_supply_state = check_supply_sat_state(var1_supply);
    bool var2_supply_state = check_supply_sat_state(var2_supply);
    long long var1_val_new = var1.allocate_value - trans_val;
    long long var2_val_new = var2.allocate_value + trans_val;

    double remain_rate_s1_before = supply_remain[var1_supply] / supply_value[var1_supply];
    double remain_rate_s2_before = supply_remain[var2_supply] / supply_value[var2_supply];
    double remain_rate_s1_after  = (supply_remain[var1_supply] + trans_val) / supply_value[var1_supply];
    double remain_rate_s2_after  = (supply_remain[var2_supply] - trans_val) / supply_value[var2_supply];

    score += abs(remain_rate_s1_before - remain_rate_s2_before) - abs(remain_rate_s1_after - remain_rate_s2_after);

    if (unsat_demand == 0 && unsat_supply == 0){
        if (obj1_supply_flag[var1_supply] == 1 && obj1_supply_flag[var2_supply] == 0){
            score += weight_obj1 * trans_val;
        }
        if (obj1_supply_flag[var1_supply] == 0 && obj1_supply_flag[var2_supply] == 1){
            score -= weight_obj1 * trans_val;
        }
    }

    return score;
}


bool MultiObjectiveData::update_solution2(double obj2_bound){
    if (best_unsat_num != 0 && unsat_demand + unsat_supply < best_unsat_num){
        best_unsat_num = unsat_demand + unsat_supply;
    }

    if (unsat_demand == 0 && unsat_supply == 0){
        long long obj1 = 0;
        double    obj2 = calcu_objective2_value();

        for (int i = 0; i < obj1_query_supply.size(); i++){
            obj1 += query_use[i];
            obj2 += abs((double) query_const_coef[i] * query_use_total - query_use[i]);
            //cout << i << " " << query_const_coef[i] << " " << query_use_total << " " << query_const_coef[i] * query_use_total << " " << query_use[i] << endl;
        }
        // cout << "obj1: " << obj1 << " " << "obj2: " << obj2 << endl;
        if (obj2_bound != 0 && obj2 > obj2_bound) return false;

        bool add_flag = false;
        if (best_obj_value_vec.size() == 0) add_flag = true;

        // cout << "all query supply: " << calcu_objective1_supply_query() << " obj1: " << obj1 << " " << " obj2: " << obj2 << endl;
        //cout << "update solution: " << obj1 << " " << calcu_objective2_value() << " " << obj2 << endl;

        // for (int i = 0; i < best_obj_value_vec.size(); i++){
        //     if (obj1 <= best_obj_value_vec[i].first && obj2 >= best_obj_value_vec[i].second){
        //         break;
        //     }

        //     if (obj1 > best_obj_value_vec[i].first && obj2 < best_obj_value_vec[i].second){ //替换当前解
        //         delete_solution(i);
        //         // best_obj_value_vec[i] = std::make_pair(obj1,obj2);
        //         // push_assignment(i);
        //         // add_flag = false;
        //         // break;
        //     }

        //     if (i == best_obj_value_vec.size() - 1){
        //         add_flag = true;
        //     }
        // }

        for (int i = 0; i < best_obj_value_vec.size(); i++){
            if (obj1 <= best_obj_value_vec[i].obj1 && obj2 >= best_obj_value_vec[i].obj2){
                break;
            }
            if ( (obj1 >= best_obj_value_vec[i].obj1 && obj2 < best_obj_value_vec[i].obj2) || (obj1 > best_obj_value_vec[i].obj1 && obj2 <= best_obj_value_vec[i].obj2)){
                best_obj_value_vec[i].delete_flag = true;
            }
            if (i == best_obj_value_vec.size() - 1){
                add_flag = true;
            }
        }
        // cout << "obj1: " << obj1 << " obj2: " << obj2 << "  " << add_flag << endl;

        // for (int supply = 0; supply < supply_cnt; supply++){
        //     for (int index = 0; index < map_supply2demand[supply].size(); index++){
        //         cout << "assign: " << assign_supply2demand[supply][index].suppy_index << " " << assign_supply2demand[supply][index].demand_index << " " << assign_supply2demand[supply][index].allocate_value << endl;
        //     }
        // }
        // for (int i = 0; i < obj1_query_supply.size(); i++){
        //     cout << "i: " << i << " " << query_use.size() << endl;
        //     cout << "query: " << obj1_query_supply[i] << " " << query_use[i] << endl;
        // }

        if (add_flag == true){   //新解添加到最后
            int position = best_obj_value_vec.size();

            Solu solu1;
            solu1.obj1 = obj1;
            solu1.obj2 = obj2;
            solu1.delete_flag = false;

            best_obj_value_vec.push_back(solu1);
            // cout << "obj1: " << obj1 << " obj2: " << obj2 << endl;
            push_assignment(position);
            
            if (!check_solution(best_obj_value_vec[position])){
                cout << "solution error!" << endl;
                getchar();
            }
        }
        delete_solution();
        if (add_flag == true) return true;
        else return false;
    }
    return false;
}

// new method for adjust the query
void MultiObjectiveData::incremental_update_solutioon(){
    if (best_unsat_num != 0 && unsat_demand + unsat_supply < best_unsat_num){
        best_unsat_num = unsat_demand + unsat_supply;
    }

    double obj2_bound = init_solution();
    long long min_query_supply = DUMMY_MAX_INT;
    long long query_provide_total = calcu_objective1_supply_query();
    vector<long long> query_provide;
    query_use_total = 0;
    query_use.clear();
    
    vector<double>    query_supply_obj2_value;  //need update
    for (int i = 0; i < obj1_query_supply.size(); i++){
        long long supply = obj1_query_supply[i];
        long long query_remain = supply_remain[supply];
        query_provide.push_back(query_remain);

        long long query_supply = query_remain / query_const_coef[i];
        //cout << query_supply << endl;
        if (query_supply < min_query_supply) min_query_supply = query_supply;
    }

    for (int i = 0; i < obj1_query_supply.size(); i++){
        long long supply = obj1_query_supply[i];
        //long long supply_use = round((double) min_query_supply * query_const_coef[i]);
        long long supply_use = 0;
        query_use.push_back(supply_use);
        query_use_total += supply_use;
        double query_obj2_value = abs((double) query_const_coef[i] * min_query_supply - supply_use); 
        query_supply_obj2_value.push_back(query_obj2_value);
        assert(supply_use <= query_provide[i]);
    }

    while (query_use_total < query_provide_total)
    {
        long long best_supply              = -1;
        long long best_index               = -1;
        double    best_obj2_increase_value = DUMMY_MAX_INT;

        for (int i = 0; i < obj1_query_supply.size(); i++){
            if (query_use[i] == query_provide[i]) continue;

            long long supply      = obj1_query_supply[i];
            double old_obj2_value = abs((double) query_const_coef[i] * (query_use_total + 1) - (query_use[i]));
            double new_obj2_value = abs((double) query_const_coef[i] * (query_use_total + 1) - (query_use[i] + 1));
            //double obj2_diff      = new_obj2_value - query_supply_obj2_value[i];
            double obj2_diff      = new_obj2_value - old_obj2_value;

            if (obj2_diff < best_obj2_increase_value){
                best_supply              = supply;
                best_index               = i;
                best_obj2_increase_value = obj2_diff;
            }
        }

        query_use[best_index]++;
        query_use_total++;
        query_supply_obj2_value[best_index] = abs((double) query_const_coef[best_index] * query_use_total - query_use[best_index]);
        if (!update_solution2(obj2_bound)) return;
    }
    query_use.clear();
}

bool MultiObjectiveData::one_time_update_solution(long long query_demand_value){

    vector<balance_coefficient> balance_vec;

    for (int i = 0; i < obj1_query_supply.size(); i++){
        long long supply_index = obj1_query_supply[i];
        long long supply_val = supply_value[supply_index];
        long long total_demand_supply_val = obj1_total_query_supply;

        balance_coefficient balance_coef;
        balance_coef.val = (double) query_const_coef[i] * query_demand_value;
        balance_coef.demand_index = 0;
        balance_coef.supply_index = supply_index;
        balance_coef.position = i;
        balance_vec.push_back(balance_coef);
    }
    std::sort(balance_vec.begin(),balance_vec.end(),cmp_balance_coef);

    query_use.clear();
    query_use_total = 0;
    query_use.resize(obj1_query_supply.size(), 0);
    for (balance_coefficient balance_coef : balance_vec){
        long long query_demand_remain = query_demand_value - query_use_total;

        if (query_demand_remain <= 0) break;
        
        else {
            long long bal_val = round(balance_coef.val);
            long long val = std::max(bal_val,(long long)1);
            val = std::min({val, query_demand_remain, supply_remain[balance_coef.supply_index]});

            long long pos = balance_coef.position;
            query_use[pos] = val;
            query_use_total += val;
        }
    }
    return update_solution2(0);
}

void MultiObjectiveData::delete_solution(){
    // long long solution_vec_size = best_obj_value_vec.size();

    // best_obj_value_vec[num1] = best_obj_value_vec[solution_vec_size - 1];
    // best_obj_value_vec.pop_back();

    // best_assign_vec[num1] = best_assign_vec[solution_vec_size - 1];
    // best_assign_vec.pop_back();
    for (int i = 0; i < best_obj_value_vec.size(); i++){
        if (best_obj_value_vec[i].delete_flag == true){
            best_obj_value_vec[i] = best_obj_value_vec[best_obj_value_vec.size() - 1];
            best_obj_value_vec.pop_back();
            i--;
        }
    }
}


void MultiObjectiveData::push_assignment(long long num1){
    // if (best_obj_value_vec.size() == num1){
    //     // best_obj_value_vec.push_back(vector<vector<allocateVar>>());
    //     best_obj_value_vec[num1].assign_vec.resize(supply_cnt + 1);
    // }
     best_obj_value_vec[num1].assign_vec.resize(supply_cnt + 1);
    assert(num1 < best_obj_value_vec.size());

    for (long long supply = 0; supply < supply_cnt; supply++){
        best_obj_value_vec[num1].assign_vec[supply].resize(assign_supply2demand[supply].size());
        for (long long index = 0; index < assign_supply2demand[supply].size(); index++){
            long long demand = map_supply2demand[supply][index];
            allocateVar var = assign_supply2demand[supply][index];
            best_obj_value_vec[num1].assign_vec[supply][index] = var;
        }
    }
}

// bool MultiObjectiveData::do_reduce_move(){
//     operatorpool.clear();

//     for (int i = 0; i < BMS; i++){
//         long long index = rand() % obj1_query_supply.size();
//         long long supply = obj1_query_supply[index];
//         index = rand() % map_supply2demand[supply].size();
//         long long demand = map_supply2demand[supply][index];

//         allocateVar var = assign_supply2demand[supply][index];

//         long long demand_remain_temp = demand_remain[demand];
//         long long supply_remain_temp = supply_remain[supply];
//         if (demand_remain_temp >= 0 || supply_remain_temp <= 0) continue;
//         long long var_min = std::min(-demand_remain_temp, var.allocate_value);
//         long long var_max = supply_remain_temp + var.allocate_value;

//         if (var.allocate_value == var_min) continue;
//         operatorpool.push(var,var_min);
//     }

//     return choose_from_operatorPool(0);
// }

bool MultiObjectiveData::do_2step_reduce_move(){

    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0, best_pos2 = 0;
    allocateVar best_var1, best_var2;

    for (int i = 0; i < BMS; i++){
        // long long index = rand() % obj1_query_supply.size();
        // long long supply_out = obj1_query_supply[index];
        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0)continue;
        index = rand() % map_demand2supply[demand].size();
        long long pos2 = allocat_position_in_supply[demand][index];
        long long supply_in = map_demand2supply[demand][index];
        if(supply_in == supply_out) continue;

        allocateVar var1 = assign_supply2demand[supply_out][pos1];
        allocateVar var2 = assign_supply2demand[supply_in][pos2];
        if (obj1_supply_flag[supply_in] == 1) continue;
        if (var1.allocate_value == 0)continue;

        long long val_trans = var1.allocate_value;
        long long cur_score = calcu_2step_operator_score(var1, var2, val_trans);

        if (cur_score > best_score){
            best_score = cur_score;
            best_trans_val = val_trans;
            best_pos1 = pos1;
            best_pos2 = pos2;
            best_var1 = var1;
            best_var2 = var2;
        }
    }

    if (best_trans_val > 0){
        // cout << " demand before change: " <<best_var1.demand_index << " " << best_var2.demand_index << " " << demand_remain[best_var1.demand_index] << endl;
        // cout << "value1: " << best_var1.allocate_value << " " << best_var1.allocate_value - best_trans_val << " " << " value2: " << best_var2.allocate_value << " " << best_var2.allocate_value + best_trans_val << endl;
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        change_assignment(best_var2.suppy_index, best_pos2, best_var2.demand_index, best_var2.allocate_value + best_trans_val);
        // cout << "two step change: " << best_trans_val << endl;
        // cout << " demand after change: " <<best_var1.demand_index << " " << best_var2.demand_index << " " << demand_remain[best_var1.demand_index] << endl;
        assert(unsat_demand == 0);
        return true;
    }
    return false;
}

bool MultiObjectiveData::do_2step_reduce_balance_move_new(){

    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0, best_pos2 = 0;
    allocateVar best_var1, best_var2;

    for (int i = 0; i < BMS; i++){
        // long long index = rand() % obj1_query_supply.size();
        // long long supply_out = obj1_query_supply[index];
        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0)continue;

        for (index = 0; index < map_demand2supply[demand].size(); index++){
            long long remain_size = map_demand2supply[demand].size() - i;

            long long pos2 = allocat_position_in_supply[demand][index];
            long long supply_in = map_demand2supply[demand][index];
            if(supply_in == supply_out) continue;

            allocateVar var1 = assign_supply2demand[supply_out][pos1];
            allocateVar var2 = assign_supply2demand[supply_in][pos2];
            if (obj1_supply_flag[supply_in] == 1) continue;
            if (var1.allocate_value == 0)continue;

            long long val_trans = std::max(var1.allocate_value / remain_size, (long long)1);
            
            best_trans_val = val_trans;
            best_pos1 = pos1;
            best_pos2 = pos2;
            best_var1 = var1;
            best_var2 = var2;
            change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
            change_assignment(best_var2.suppy_index, best_pos2, best_var2.demand_index, best_var2.allocate_value + best_trans_val);
               
        }
    }

    assert(unsat_demand == 0);
    return true;
}


bool MultiObjectiveData::check_solution(Solu solu){
    vector<long long> demand_vec(demand_cnt + 1, 0);
    vector<long long> supply_vec(supply_cnt + 1, 0);

    for (long long supply = 0; supply < supply_cnt; supply++){
        for (long long pos = 0; pos < map_supply2demand[supply].size(); pos++){
            long long demand = map_supply2demand[supply][pos];
            allocateVar var = solu.assign_vec[supply][pos];
            // cout << "var.suppy_index == supply  " << var.suppy_index << "  " << var.demand_index << " " << supply << " " << demand << endl;
            assert(var.suppy_index == supply);
            assert(var.demand_index == demand);
            demand_vec[demand] += var.allocate_value;
            supply_vec[supply] += var.allocate_value;
        }
    }

    for (long long supply = 0; supply < supply_cnt; supply++){
        if (supply_vec[supply] > supply_value[supply]) return false;
    }
    for (long long demand = 0; demand < demand_cnt; demand++){
        if (demand_vec[demand] < demand_value[demand]) return false;
    }
    return true;
}

struct ParetoSolution
{
    long long obj1;
    double    obj2;
};
bool ParetoSolution_cmp(ParetoSolution ps1, ParetoSolution ps2){
    return ps1.obj1 < ps2.obj1;
}

double MultiObjectiveData::hyper_volume(){

    vector<ParetoSolution> solu_vec;
    double hypervolume = 0;

    ParetoSolution paretosolu1;
    paretosolu1.obj1 = 0;
    paretosolu1.obj2 = 0;
    solu_vec.push_back(paretosolu1);

    for (int i = 0; i < best_obj_value_vec.size(); i++){
        ParetoSolution paretosolu1;
        paretosolu1.obj1 = best_obj_value_vec[i].obj1;
        paretosolu1.obj2 = best_obj_value_vec[i].obj2;
        solu_vec.push_back(paretosolu1);
    }
    std::sort(solu_vec.begin(),solu_vec.end(),ParetoSolution_cmp);

    long long upperbound = 1000000;

    for (int i = 0; i < solu_vec.size(); i++){
        double volume = 0;
        if (i == 0) volume = (upperbound - solu_vec[i].obj2) * solu_vec[i].obj1;
        else {
            volume = (upperbound - solu_vec[i].obj2) * (solu_vec[i].obj1 - solu_vec[i - 1].obj1);
        }
        hypervolume += volume;
    }

    cout << "HyperVolume: " << hypervolume << endl;
    return hypervolume;
}

int MultiObjectiveData::objective_order(){

    int obj1_degree = 0;
    int obj2_degree = 0;

    for(auto supply : obj1_query_supply){
        obj1_degree++;
    }

    for(long long demand_index = 0; demand_index < demand_cnt; demand_index++){
        for(long long i = 0; i < map_demand2supply[demand_index].size(); i++){
            obj2_degree += 2;
        }
    }
    cout << "Obj1 ConDegree: " << obj1_degree << endl;
    cout << "Obj2 ConDegree: " << obj2_degree << endl;

    if (obj1_degree >= obj2_degree) return 2;
    else return 1;
}

bool MultiObjectiveData::do_2step_improve_move(){

    allocateVar best_var1, best_var2;

    for (int i = 0; i <= BMS; i++){

        if (i == BMS) return false;
        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0)continue;

        index = rand() % map_demand2supply[demand].size();

        long long remain_size = map_demand2supply[demand].size();
        long long pos2 = allocat_position_in_supply[demand][index];
        long long supply_in = map_demand2supply[demand][index];
        

        allocateVar var1 = assign_supply2demand[supply_out][pos1];
        allocateVar var2 = assign_supply2demand[supply_in][pos2];

        long long val_trans = std::max(var1.allocate_value / remain_size, (long long)1);

        double coef1 = obj2_const_coef[supply_out][pos1];
        double coef2 = obj2_const_coef[supply_in][pos2];
        double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - val_trans - coef1);
        double diff2 = abs(var2.allocate_value - coef2) - abs(var2.allocate_value + val_trans - coef2);

        if (obj1_supply_flag[supply_in] == 1) continue;
        if (var1.allocate_value == 0)continue;
        if(supply_in == supply_out) continue;
        if (diff1 + diff2 < 0) continue;
        
        long long best_trans_val = val_trans;
        long long best_pos1 = pos1;
        long long best_pos2 = pos2;
        best_var1 = var1;
        best_var2 = var2;

        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        change_assignment(best_var2.suppy_index, best_pos2, best_var2.demand_index, best_var2.allocate_value + best_trans_val);
        break;
            
    }

    assert(unsat_demand == 0);
    return true;
}

bool MultiObjectiveData::do_1step_improve_move(){

    allocateVar best_var1, best_var2;

    for (int i = 0; i <= BMS; i++){

        if (i == BMS) return false;
        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0) continue;
        if (demand_remain[demand] >= 0) continue;

        index = rand() % map_demand2supply[demand].size();

        long long remain_size = map_demand2supply[demand].size() - i;

        allocateVar var1 = assign_supply2demand[supply_out][pos1];

        long long val_trans = std::max(var1.allocate_value / remain_size, (long long)1);
        val_trans = std::min(val_trans, -demand_remain[demand]);

        double coef1 = obj2_const_coef[supply_out][pos1];
        double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - val_trans - coef1);

        if (var1.allocate_value == 0)continue;
        if (diff1 < 0) continue;
        
        long long best_trans_val = val_trans;
        long long best_pos1 = pos1;
        best_var1 = var1;
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        break;
    }

    assert(unsat_demand == 0);
    return true;
}

bool MultiObjectiveData::do_2step_reduce_move_new(){

    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0, best_pos2 = 0;
    allocateVar best_var1, best_var2;
    bool find_best_flag = false;

    for (int i = 0; i < BMS; i++){

        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0)continue;

        for (index = 0; index < map_demand2supply[demand].size(); index++){
            long long remain_size = map_demand2supply[demand].size();

            long long pos2 = allocat_position_in_supply[demand][index];
            long long supply_in = map_demand2supply[demand][index];
            if(supply_in == supply_out) continue;

            allocateVar var1 = assign_supply2demand[supply_out][pos1];
            allocateVar var2 = assign_supply2demand[supply_in][pos2];
            if (obj1_supply_flag[supply_in] == 1) continue;
            if (var1.allocate_value == 0)continue;

            long long val_trans = std::max(var1.allocate_value / remain_size, (long long)1);

            double coef1 = obj2_const_coef[supply_out][pos1];
            double coef2 = obj2_const_coef[supply_in][pos2];
            double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - val_trans - coef1);
            double diff2 = abs(var2.allocate_value - coef2) - abs(var2.allocate_value + val_trans - coef2);
            double delta2 = diff1 + diff2;

            double score = val_trans / abs(delta2);

            if (score > best_score){
                find_best_flag = true;
                best_score = score;
                best_trans_val = val_trans;
                best_pos1 = pos1;
                best_pos2 = pos2;
                best_var1 = var1;
                best_var2 = var2;
                if (delta2 > 0) break;
            }
        }
    }

    assert(unsat_demand == 0);
    if (find_best_flag == true){
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        change_assignment(best_var2.suppy_index, best_pos2, best_var2.demand_index, best_var2.allocate_value + best_trans_val);
        return true;
    }
    else{
        return false;
    }
}

bool MultiObjectiveData::do_1step_reduce_move_new(){

    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0;
    allocateVar best_var1;
    bool find_best_flag = false;

    for (int i = 0; i <= BMS; i++){

        if (i == BMS) return false;
        if (query_supply_not_full.empty()) return false;

        long long index = rand() % query_supply_not_full.size();
        long long supply_out = query_supply_not_full[index];

        if (map_supply2demand[supply_out].size() == 0) continue; 
        long long pos1 = rand() % map_supply2demand[supply_out].size();
        long long demand = map_supply2demand[supply_out][pos1];
        if (map_demand2supply[demand].size() == 0) continue;
        if (demand_remain[demand] >= 0) continue;

        index = rand() % map_demand2supply[demand].size();

        long long remain_size = map_demand2supply[demand].size() - i;

        allocateVar var1 = assign_supply2demand[supply_out][pos1];

        long long val_trans = std::max(var1.allocate_value / remain_size, (long long)1);
        val_trans = std::min(val_trans, -demand_remain[demand]);

        double coef1 = obj2_const_coef[supply_out][pos1];
        double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - val_trans - coef1);
        double score = val_trans / abs(diff1);

        if (var1.allocate_value == 0)continue;

        if (score > best_score){
            find_best_flag = true;
            best_score = score;
            best_trans_val = val_trans;
            best_pos1 = pos1;
            best_var1 = var1;
            if (diff1 > 0) break;
        }
    }

    assert(unsat_demand == 0);
    if (find_best_flag == true){
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        return true;
    }
    else{
        return false;
    }
}

bool MultiObjectiveData::do_1step_reduce_move_new_bal2(){
    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0, best_pos2 = 0;
    allocateVar best_var1, best_var2;
    bool find_best_flag = false;

    for(int i = 0; i <= BMS; i++){
        if (i == BMS) return false;

        long long supply = rand() % supply_cnt;
        if (map_supply2demand[supply].size() == 0) continue;
        long long pos = rand() % map_supply2demand[supply].size();
        long long demand = map_supply2demand[supply][pos];

        allocateVar var1 = assign_supply2demand[supply][pos];
        double coef = obj2_const_coef[supply][pos];
        long long trans_val = 0;
        
        if (coef > var1.allocate_value){
            trans_val = round(coef - var1.allocate_value);
            trans_val = std::min(trans_val, supply_remain[supply]);
        }
        else {
            trans_val = round(coef - var1.allocate_value);
            trans_val = std::max(trans_val, demand_remain[demand]);
            if(abs(trans_val) > var1.allocate_value){
                trans_val = -var1.allocate_value;
            }
        }
        if (trans_val == 0)continue;

        long long delta1 = 0;
        if (true){

            double coef1 = obj2_const_coef[supply][pos];
            double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - trans_val - coef1);
            double delta2 = diff1;
            delta1 = trans_val;

            if (obj1_supply_flag[supply] == 0){
                delta1 = 1;
            }

            if (delta2 <= 0) continue;

            double score = delta2 / delta1;
            if (score > best_score){
                find_best_flag = true;
                best_score = score;
                best_trans_val = trans_val;
                best_pos1 = pos;
                best_var1 = var1;
            }
        }
        else{
            find_best_flag = true;
            best_trans_val = trans_val;
            best_pos1 = pos;
            best_var1 = var1;
            break;
        }
    }

    if (find_best_flag == true){
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value + best_trans_val);
        return true;
    }
    else{
        return false;
    }
    
}

bool MultiObjectiveData::do_2step_reduce_move_new_bal2(long long demand){

    vector<long long> val_need_increase;    //restore pos
    vector<long long> val_need_decrease;    //restore pos

    for (long long pos = 0; pos < map_demand2supply[demand].size(); pos++){
        long long supply = map_demand2supply[demand][pos];
        long long supply_pos = allocat_position_in_supply[demand][pos];
        // if (obj1_supply_flag[supply] == 1) continue;
        if (obj2_const_coef[supply][supply_pos] - assign_supply2demand[supply][supply_pos].allocate_value > 0 && supply_remain[supply] > 0){
            val_need_increase.push_back(pos);
        }
        if (obj2_const_coef[supply][supply_pos] - assign_supply2demand[supply][supply_pos].allocate_value < -1){
            val_need_decrease.push_back(pos);
        }
    }

    long long best_score = NEGATIVE_INFINITY;
    long long best_trans_val = 0;
    long long best_pos1 = 0, best_pos2 = 0;
    allocateVar best_var1, best_var2;
    bool find_best_flag = false;

    if (val_need_decrease.empty() || val_need_increase.empty()) return false;

    for(int i = 0; i < BMS; i++)
    {
        long long pos1_demand = val_need_decrease.back();   //1 - decrease
        long long pos2_demand = val_need_increase.back();   //2 - increase
        val_need_decrease.pop_back();
        val_need_increase.pop_back();

        long long supply1 = map_demand2supply[demand][pos1_demand];
        long long supply2 = map_demand2supply[demand][pos2_demand];
        long long pos1_supply = allocat_position_in_supply[demand][pos1_demand];
        long long pos2_supply = allocat_position_in_supply[demand][pos2_demand];

        long long val1 = assign_supply2demand[supply1][pos1_supply].allocate_value - obj2_const_coef[supply1][pos1_supply];
        long long val2 = supply_remain[supply2];
        long long trans_val = std::min(val1, val2);

        allocateVar var1 = assign_supply2demand[supply1][pos1_supply];
        allocateVar var2 = assign_supply2demand[supply2][pos2_supply];

        long long delta1 = 0;
        if (obj1_supply_flag[supply2] == 1 && obj1_supply_flag[supply1] == 0){

            double coef1 = obj2_const_coef[supply1][pos1_supply];
            double coef2 = obj2_const_coef[supply2][pos2_supply];
            double diff1 = abs(var1.allocate_value - coef1) - abs(var1.allocate_value - trans_val - coef1);
            double diff2 = abs(var2.allocate_value - coef2) - abs(var2.allocate_value + trans_val - coef2);
            double delta2 = diff1 + diff2;
            delta1 = trans_val;

            if (delta2 <= 0) continue;

            double score = delta2 / delta1;
            if (score > best_score){
                find_best_flag = true;
                best_score = score;
                best_trans_val = trans_val;
                best_pos1 = pos1_supply;
                best_pos2 = pos2_supply;
                best_var1 = var1;
                best_var2 = var2;
            }
        }
        else{
            find_best_flag = true;
            best_trans_val = trans_val;
            best_pos1 = pos1_supply;
            best_pos2 = pos2_supply;
            best_var1 = var1;
            best_var2 = var2;
            break;
        }
        

        // change_assignment(supply1, pos1_supply, demand, assign_supply2demand[supply1][pos1_supply].allocate_value - trans_val);
        // change_assignment(supply2, pos2_supply, demand, assign_supply2demand[supply2][pos2_supply].allocate_value + trans_val);

        // if (obj2_const_coef[supply1][pos1_supply] - assign_supply2demand[supply1][pos1_supply].allocate_value < -1){
        //     val_need_decrease.push_back(pos1_demand);
        // }
        // if (obj2_const_coef[supply2][pos2_supply] - assign_supply2demand[supply2][pos2_supply].allocate_value > 0 && supply_remain[supply2] > 0){
        //     val_need_increase.push_back(pos2_demand);
        // }

    }

    if (find_best_flag == true){
        change_assignment(best_var1.suppy_index, best_pos1, best_var1.demand_index, best_var1.allocate_value - best_trans_val);
        change_assignment(best_var2.suppy_index, best_pos2, best_var2.demand_index, best_var2.allocate_value + best_trans_val);
        return true;
    }
    else{
        return false;
    }

}
