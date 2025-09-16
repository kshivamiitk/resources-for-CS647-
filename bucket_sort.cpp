#include<iostream>
#include<vector>
using namespace std;
void insertionSort(vector<int>&arr){
    for(int i = 0 ; i < arr.size() ; i++){
        for(int j = i + 1 ; j < arr.size() ; i++){
            if(arr[i] > arr[j])swap(arr[i] , arr[j]);
        }
    }
}
vector<int> BucketSort(vector<int>arr, int k){
    // makee buckets of size k and then sort them out 
    int minm = *min_element(arr.begin() , arr.end());
    int maxm = *max_element(arr.begin() , arr.end());
    // we are supposed to have k buckets only
    // so the buckest would have values minm + (maxm - minm) * i / k;
    vector<vector<int> > buckets(k + 3);
    for(int i = 0 ; i < arr.size() ; i++){
        if(!maxm==minm)
        {int j = ((arr[i] - minm) * k) / (maxm - minm);
        buckets[j].push_back(arr[i]);}
    }
    for(int i = 0 ; i <= k+2 ; i++){
        insertionSort(buckets[i]);
    }
    // merge the elements
    vector<int>answer;
    for(int i = 0 ; i <= k+2 ; i++){
        for(int j = 0 ; j < buckets[i].size() ; j++){
            answer.push_back(buckets[i][j]);
        }
    }
    return answer;
}
int main(){
    int n ;
    cin >> n;
    int k;
    cin >> k;
    vector<int>arr(n);
    for(int i = 0 ; i < n ; i++){
        cin >> arr[i];
    }
    vector<int> answer = BucketSort(arr,k);
    for(int i = 0 ; i < answer.size() ; i++){
        cout << answer[i] << " " ;
    }
}