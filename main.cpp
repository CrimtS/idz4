#include <pthread.h>
#include <future>
#include <deque>
#include <random>
#include <memory>
#include <iostream>
#include <cstdio>

using namespace std;
string goods[3][4] =
        {{"Adderall",  "Aspirin",    "Paracetamol", "Glycine"},
         {"Milk",      "Sour Cream", "Cheddar",     "Cottage Cheese"},
         {"Spaghetti", "Fettuccine", "Pappardelle", "Bavette"}};

class Customer {
public:
    int ID{};
    deque<int> deps_to_go = deque<int>(0);

    Customer() {
        ID = 0;
        deps_to_go = deque<int>();
    }

    Customer(int ID, const deque<int> &to_go) {
        this->ID = ID;
        for (int i: to_go) {
            deps_to_go.push_back(i);
        }
    }
};

class Seller {
public:
    deque<Customer> queue = deque<Customer>(0);
    bool is_busy = false;

    Seller() {
        pthread_mutex_init(&mutex, nullptr);
    }

    pthread_mutex_t mutex{};

    ~Seller() {
        pthread_mutex_destroy(&mutex);
    }
};

Seller sellers[3];
deque<Customer> customers;

void SellingThread(int id) {
    Customer customer = customers[id];
    int aisle_id = customer.deps_to_go[0];
    while (sellers[aisle_id].is_busy) {
        this_thread::sleep_for(std::chrono::seconds(1));
    }
    pthread_mutex_lock(&sellers[aisle_id].mutex);
    sellers[aisle_id].is_busy = true;
    random_device rd;
    uniform_int_distribution<size_t> u_dist(0, 3);
    this_thread::sleep_for(std::chrono::seconds(2));
    string good = goods[aisle_id][u_dist(rd)];
    printf("Customer %d bought %s in aisle %d \n", customer.ID + 1, good.c_str(), customer.deps_to_go[0] + 1);
    customer.deps_to_go.pop_front();
    if (customer.deps_to_go.empty()) {
        customers.pop_front();
    } else {
        customers.pop_front();
        printf("Customer %d going to aisle %d \n", customer.ID + 1, customer.deps_to_go[0] + 1);
        customers.emplace_back(customer.ID, customer.deps_to_go);
    }
    pthread_mutex_unlock(&sellers[aisle_id].mutex);
    sellers[aisle_id].is_busy = false;
}

void *sellThread(void *args) {
    int *customerID = (int *) args;
    SellingThread(*(customerID));
    return nullptr;
}


void StartDay() {
    while (!customers.empty()) {
        deque<pthread_t> customerThreads = deque<pthread_t>(customers.size());
        for (int i = 0; i < customers.size(); i++) {
            pthread_create(&customerThreads[i], nullptr, sellThread, &customers[i].ID);
        }
        for (auto thread: customerThreads) {
            pthread_join(thread, nullptr);
        }
    }
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    cout << "Enter the number of customers today:";
    int customers_number;
    cin >> customers_number;
    while (customers_number < 0) {
        cout << "You entered a netgative number, please retry:";
        cin >> customers_number;
    }
    cout << "For each customer enter ID's of aisles for purchases (aisles 1,2,3)"
            "in wanted order. \nIf customer does NOT want to go to all three aisles, put zeros AFTER wanted ID's. \n"
            "(Code is not reading input after zero, so 0 1 2 is not suitable)\n"
            "Examples: 1 2 3, 3 1 2, 3 2 1, 3 0 0, 1 2 0, 1 0 0, 3 2 0\n";
    for (int i = 0; i < customers_number; ++i) {
        Customer temp;
        temp.ID = i;
        int a[3];
        cin >> a[0] >> a[1] >> a[2];
        while (true) {
            if (a[0] <= 0 || a[0] > 3 || a[1] <= 0 || a[1] > 3 || a[2] <= 0 || a[2] > 3) {
                cout << "You entered incorrect line of aisles, please retry";
                cin >> a[0] >> a[1] >> a[2];
                continue;
            }
            break;

        }
        for (auto aisle: a) {
            if (aisle == 0) {
                break;
            }
            temp.deps_to_go.push_back(aisle - 1);
        }
        if (!temp.deps_to_go.empty())
            customers.push_back(temp);
    }
    for (const auto &c: customers) {
        cout << "ID " << c.ID << " ";
        for (auto v: c.deps_to_go) {
            cout << v << " ";
        }
        cout << "\n";
    }
    StartDay();
}