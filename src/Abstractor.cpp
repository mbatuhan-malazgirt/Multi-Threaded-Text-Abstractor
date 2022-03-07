#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iomanip>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <string.h>
#include <set>
#include <pthread.h>
#include <algorithm>
#include <deque>
using namespace std;

ofstream out;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct jaccard_similarity_struct
{
    string name;
    string query;
    string abstract;
    string abstract_name;
    string summary;
    double result;
};


void* jaccard_similarity(void* arg)
{
	   
    struct jaccard_similarity_struct* arg_struct = (struct jaccard_similarity_struct*)arg;

    vector <string> query_words;
    deque<string> sentences;
    string abstract_all = arg_struct->abstract;
    set<string> abstract_token_set;
    set<string> query_set;
    string word;
    string sentence="";
    string summary="";
    istringstream iss(abstract_all);
    while (iss >> word)
    {
	string word_2 = word + " ";
	sentence += word_2;
	if ( !( word.compare(".") ) )
	{
		sentences.push_back(sentence);
		sentence =  "";
	}
        abstract_token_set.insert(word);
    }

    istringstream iss_one(arg_struct->query);
    while (iss_one >> word)
    {
	query_words.push_back(word);
        query_set.insert(word);
    }

   while( !( sentences.empty() ) )
  {
	bool there_is_a_problem_washington = false;
	string sentence = sentences.front();
	sentences.pop_front();
	vector<string> ::iterator it;

	 for(it=query_words.begin();it!=query_words.end();it++)
   	 {
        	size_t found = sentence.find(*it);
		cout << found << endl;
		if ( (found!= string::npos)  &&  sentence.at(found-1)==' '  )
		{
			
			 there_is_a_problem_washington =true;
			
		}
   	 }
	if( there_is_a_problem_washington )
	{
	summary = sentence + " ";
	}
   }

	arg_struct->summary = summary;

    vector<string> intersection;
    vector<string> token_union;
    set_intersection(abstract_token_set.begin(), abstract_token_set.end(), query_set.begin(), query_set.end(), std::back_inserter(intersection));
    set_union(abstract_token_set.begin(), abstract_token_set.end(), query_set.begin(), query_set.end(), std::back_inserter(token_union));

 arg_struct->result = (double) intersection.size() / (double) token_union.size();

 pthread_mutex_lock(&mutex);

out << "Thread " << arg_struct->name << " is calculating " << arg_struct->abstract_name << endl;

 pthread_mutex_unlock(&mutex);

    pthread_exit(0);
    
}


int main(int argc, char **argv)
{

	
    ifstream in;
 
    string input_file_name = argv[1];
    string output_file_name = argv[2];

    in.open(input_file_name);
    out.open(output_file_name, ios::app);

    string line;
    getline(in, line);
    istringstream iss(line);	
    string word;

   int number_of_threads;
   int number_of_abstracts_scanned;
   int number_of_abstracts_returned;

   iss >> number_of_threads >> number_of_abstracts_scanned >> number_of_abstracts_returned;

    getline(in, line);
    string query=line;


string names_of_abstracts[number_of_abstracts_scanned];
int q=0;

 while(getline(in, line))
{
	names_of_abstracts[q]=line;
	q++;
}

int size_of_abstracts = q;
string abstractss[number_of_abstracts_scanned];

int t=0;
while(t<size_of_abstracts)
{
	  ifstream in_abstract;
	 in_abstract.open("../abstracts/"+names_of_abstracts[t]);
	stringstream buffer;
	buffer << in_abstract.rdbuf();
	abstractss[t]= buffer.str();
	t++;
}


	   string alphabet[26];
  	  for (int i = 0; i < 26; i++) 
 	   {
   	     alphabet[i] = 65 + i;
 	   }

	

	//double all_results[number_of_abstracts_scanned];
	vector<double> all_results(number_of_abstracts_scanned);
	vector<string> all_abstract_names(number_of_abstracts_scanned);
	vector <string> all_summaries(number_of_abstracts_scanned);
	struct jaccard_similarity_struct args[number_of_threads];
   	 pthread_t tids[number_of_threads];
	
//	int original_abstracts_scanned = number_of_abstracts_scanned;
	
	int k=0;
        int r = 0;
	int number_of_active_threads = 0;
	while(number_of_abstracts_scanned>0)
	{
	
	   for (int i = 0; i < number_of_threads; i++)
        {
            args[i].query = query;
	   args[i].abstract_name= names_of_abstracts[k];
            args[i].abstract = abstractss[k];
	   k++;
            args[i].name = alphabet[i];
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_create(&tids[i], &attr, jaccard_similarity, &args[i]);
	    number_of_active_threads++;
	    number_of_abstracts_scanned--;
	    if (number_of_abstracts_scanned<=0)
	   {
		break;
	   }
        }

        for (int i = 0; i<number_of_active_threads; i++)
        {
            pthread_join(tids[i], NULL);
	   all_results.at(r)=args[i].result;
	   all_abstract_names.at(r) = args[i].abstract_name;
	   all_summaries.at(r) = args[i].summary;
	    r++;
        }
	  number_of_active_threads = 0;

	}
	

	for(int i =0 ; i < number_of_abstracts_returned  ; i++)
	 {
		
		out << "###" << endl;
		 double max = *max_element(all_results.begin(), all_results.end());
		int index = max_element(all_results.begin(), all_results.end()) - all_results.begin();
		*max_element(all_results.begin(), all_results.end()) = -1;
		out << "Result " << i+1  << ":"<< endl;
		out << "File: " << all_abstract_names.at(index) << endl;
		out << "Score: " << fixed  << setprecision(4) <<max << endl;
		out << "Summary: " << all_summaries.at(index)<< endl;
		
 	  }
	
	out << "###" << endl;
	

return 0;
}

