#include <pthread.h>
#include <unistd.h>
#include <deque>
#include <random>
#include <memory>
#include <iostream>
#include <cstdio>

using namespace std;
// Массив товаров, в каждом отделе их четыре.
string goods[3][4] =
        {{"Adderall",  "Aspirin",    "Paracetamol", "Glycine"},
         {"Milk",      "Sour Cream", "Cheddar",     "Cottage Cheese"},
         {"Spaghetti", "Fettuccine", "Pappardelle", "Bavette"}};

// Класс покупателя, у него есть уникальное ID и очередь отделов, в которые ему еще надо попасть в определенном порядке
// (deps_to_go)
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

// Класс продавца, содержащий в себе логическую переменную, занят ли он и мьютекс, обеспечивающий тот факт, чтобы
// два покупателя не могли одновременно занять одну кассу. В деструкторе мьютекс уничтожается(в конце программы).
class Seller {
public:
    bool is_busy = false;

    Seller() {
        pthread_mutex_init(&mutex, nullptr);
    }

    pthread_mutex_t mutex{};

    ~Seller() {
        pthread_mutex_destroy(&mutex);
    }
};

// Три продавца магазина
Seller sellers[3];

// Очередь покупателей
deque<Customer> customers;


// Основная функция для покупки, принимает уникальный ID покупателя и соотв. потока
void Buying(int id) {
    Customer customer;
    //Собственно, покупатель
    for (auto c: customers) {
        if (c.ID == id) {
            customer = c;
        }
    }
    // ID продавца, где покупатель стоит в очереди
    int aisle_id = customer.deps_to_go.front();
    customer.deps_to_go.pop_front();
    // Пока продавец занят, покупатель "спит"
    while (sellers[aisle_id].is_busy) {
        sleep(1);
    }
    //Перед началом работы с продавцом мьютекс лочится, чтобы остальные не могли попасть на эту же кассу
    pthread_mutex_lock(&sellers[aisle_id].mutex);
    sellers[aisle_id].is_busy = true;

    //Получаем случайное ID товара, который покупатель покупает в отделе
    random_device rd;
    uniform_int_distribution<size_t> u_dist(0, 3);
    sleep(2);
    string good = goods[aisle_id][u_dist(rd)];
    //Покупка
    printf("Customer %d bought %s in aisle %d \n", customer.ID + 1, good.c_str(), aisle_id + 1);
    //Удаляем текущий отдел из списка покупателя, ведь в нем он уже закупился
    //Проверяем, есть ли еще отделы у него в списке. Если есть, то покупатель становится в очередь в этот отдел
    if (customer.deps_to_go.empty()) {
        customers.pop_front();
    } else {
        customers.pop_front();
        printf("Customer %d going to aisle %d \n", customer.ID + 1, customer.deps_to_go[0] + 1);
        customers.emplace_back(Customer(customer.ID, customer.deps_to_go));
    }
    //Разблокируем мьютекс и меняем логическую переменную, касса свободна для следующего покупателя
    pthread_mutex_unlock(&sellers[aisle_id].mutex);
    sellers[aisle_id].is_busy = false;
}

//Threading-функция, которая вызывает Buying, передавая ID покупателя
void *BuyThreading(void *args) {
    int *customerID = (int *) args;
    Buying(*(customerID));
    return nullptr;
}

//Функция, запускающая магазин в работу. Создается список потоков покупок, которые джойнятся после завершения.
//Это повторяется, пока очередь покупок не опустеет.
void StartDay() {
    while (!customers.empty()) {
        deque<pthread_t> customerThreads = deque<pthread_t>(customers.size());
        for (int i = 0; i < customers.size(); i++) {
            pthread_create(&customerThreads[i], nullptr, BuyThreading, &customers[i].ID);
        }
        for (auto thread: customerThreads) {
            pthread_join(thread, nullptr);
        }
    }
}

//main
int main() {
    //Ввод числа покупателей в день, оно не может быть отрицательным
    cout << "Enter the number of customers today:";
    int customers_number;
    cin >> customers_number;
    while (customers_number < 0) {
        cout << "You entered a netgative number, please retry:";
        cin >> customers_number;
    }
    //Ввод и проверка списков от жены каждого покупателя(отделов по порядку) и добавление их в очередь
    //Списки не могут начинаться с 0, а также содержать отрицательные значения, значения >3
    cout << "For each customer enter ID's of aisles for purchases (aisles 1,2,3)"
            "in wanted order. \nIf customer does NOT want to go to all three aisles, put -1 AFTER wanted ID's. \n"
            "(Code is not reading input after zero, so 0 1 2 is not suitable)\n"
            "Examples: 1 2 3, 3 1 2, 3 2 1, 3 0 0, 1 2 0, 1 0 0, 3 2 0\n";
    for (int i = 0; i < customers_number; ++i) {
        Customer temp;
        temp.ID = i;
        int a[3];
        cin >> a[0] >> a[1] >> a[2];
        while (true) {
            if (a[0] <= -1 || a[0] > 3 || a[1] < -1 || a[1] > 3 || a[2] < -1 || a[2] > 3) {
                cout << "You entered incorrect line of aisles, please retry";
                cin >> a[0] >> a[1] >> a[2];
                continue;
            }
            break;

        }
        for (auto aisle: a) {
            if (aisle != 0)
                temp.deps_to_go.push_back(aisle - 1);
        }
        if (!temp.deps_to_go.empty())
            customers.push_back(temp);
    }
    for (auto c: customers) {
        cout << "ID " << c.ID << " ";
        for (auto id: c.deps_to_go) {
            cout << id << " ";
        }
        cout << endl;
    }
    //Запуск магазина в работу, ведь покупатели уже пришли
    StartDay();
}
