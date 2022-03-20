#include <time.h>
#include <iostream>

using namespace std;


int main(){
    time_t tm = time(NULL);
    cout<<to_string(tm)<<endl;

}