#include<iostream>
#include<string>
#include<math.h>
using namespace std;

long isplit(string str){
    int i = 3,pos = 0;
    long num = 0;
    while(i){
        cout<<str<<endl;
        pos = str.find('.');
        cout<<pos<<endl;
        cout<<str.substr(0,pos)<<endl;
        num = pow(10,pos)*num + stoi(str.substr(0,pos));
        str = str.substr(pos+1);
        i--;
        cout<<num<<endl;
    }
    num = pow(10,str.size())*num + stoi(str);
    return num;
}

int main(){
	string ip = "192.168.68.139";
	cout<<isplit(ip)<<endl;
	return 0;
}
