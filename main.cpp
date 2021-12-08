// check command line arguments
// construct classifier ADT
    /*
     map is set of key, value pairs (use STD map)
     */

#include <cassert>  //assert
#include <iostream> //ostream
#include <functional> //less
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <map>
#include <cstring>
#include <set>
#include <cmath>
#include "csvstream.h"
#include "oleanderstemminglibrary/include/olestem/stemming/english_stem.h"

using namespace std;
// sets precision for outputting stuff   WHY DOES IT PRODUCE AN ERROR????
//cout.precision(3);                    //must include iomanip

class Classifier {
    
private:
    // training data info
    int numPosts = 0;
    int numWords = 0;
    
    // map to store pairs of {word, #postsWithWord} = word frequencies
    map<string, int> wordFreq;
    // map to store keyVals of {label, numPosts with that label}
    map<string, int> labelFreq;
    // map: { label that has word (keyVal), numPosts (value) with label/word pair}
    map<string,map<string,int>> trainedMap;
    // my_map[label][word]= numPosts ->initializer list represents key within map
    static double function(double x) {
        //return log(x)/log(2.5)+1;
        return 1; //return 1 for normal functionality
    }
    static set<string> unique_words(const string &str) {
 // Fancy modern C++ and STL way to do it
    istringstream source{str};
    return {istream_iterator<string>{source},
         istream_iterator<string>{}};
    }
    static map<string,pair<double,int>> words_importance(const string &str) { //returns (word,importance,frequency)
        istringstream source(str);
        map<string,pair<double,int>> words_imp;
        string word;
        while (source >> word) {
                ++words_imp[word].second;
                words_imp[word].first = function(words_imp[word].second); 
            }
        return words_imp;
    }
    double log_prior(int labelfreq) const {
        double prob_prior = labelfreq/(double)numPosts;
        return log(prob_prior);
    }
    double log_likelihood(string label,string word) {
        int numpairs = trainedMap[label][word];
        double prob_likelihood;
        if(numpairs == 0) {
            int wordfreq = wordFreq[word];
            if(wordfreq == 0) {
                return log(1/(double)numPosts);
            }
            else {
                prob_likelihood = wordfreq/(double)numPosts;
                return log(prob_likelihood);
            }
        }
        prob_likelihood = numpairs/(double)labelFreq[label];
        return log(prob_likelihood);
    }

public:
    void output_line(string &correct,string &pred,const double score, string &content) {
      cout.precision(3);
      cout<< "  correct = " << correct << ", " << 
      "predicted = " << pred << ", " << 
      "log-probability score = " << score << endl;

      cout << "  content = " << content << endl;
      cout << endl;
    }

    pair<int,int> readTest(csvstream &csvIn) {
        map<string,string> row;
        int numcorrect = 0;
        int numtestposts = 0;
        cout << "test data:" << endl;

        while (csvIn >> row) {
            string mostlikelylabel = "";
            double highest_logprob = -1 * numeric_limits<double>::infinity();
            string &post = row["content"];
 
            map<string,pair<double,int>> unwds = words_importance(post); // was set<string> unwds = unique_words(post);
            double logprob = 0;
            
            for(auto &pair: labelFreq) {
                const string &label = pair.first;
                logprob += log_prior(pair.second);

            for(auto &wd:unwds) {
                logprob += (log_likelihood(label,wd.first)) * wd.second.first; // was logprob += log_likelihood(label,wd)
            }
            if (logprob > highest_logprob) {
                highest_logprob = logprob;
                mostlikelylabel = label;
            }
            logprob = 0;
            }
            string &right = row["tag"];
            if (mostlikelylabel == right) {
                ++numcorrect;
            }
            ++numtestposts;
            output_line(right,mostlikelylabel,highest_logprob,post);
            highest_logprob = -1 * numeric_limits<double>::infinity();
            mostlikelylabel = "";
        }

        return {numcorrect,numtestposts};
    }
    // read in posts and train classifier
    void readTrain(csvstream &csvIn, bool debug) {
        cout.precision(3);
        map<string, string> row;
        //training 
        if(debug) {
            cout<<"training data:" << endl;
        }
        while (csvIn >> row) {
            ++numPosts;
            const string &post = row["content"];
            const string &label = row["tag"];
            map<string,pair<double,int>> uniqwords = words_importance(post); //was set<string> uniqwords = unique_words(post);

            for(auto &word: uniqwords) {
                for(int i=0; i<1;i++) { //used to not have this for loop,use i<1 for normal functionality
                ++wordFreq[word.first];
                ++trainedMap[label][word.first];
                }
            }
            ++labelFreq[label];
            if(debug) {
            cout<< "  label = " << label << ", content = " 
            << post << endl;
            }
        }
        numWords = wordFreq.size();

        cout<<"trained on " << numPosts << " examples" << endl;
        if(debug) {
        cout<<"vocabulary size = " << numWords << endl;
        cout << endl;
        cout<<"classes:" << endl;

        for(auto &lfpair: labelFreq) {
            cout << "  " << lfpair.first << ", " <<
            lfpair.second << " examples, log-prior = "
            << log_prior(lfpair.second)<<endl;
        }
        cout<<"classifier parameters:" << endl;

        for(auto &lmpair: trainedMap) {
            int lf = labelFreq[lmpair.first];
            for(auto &wfpair: lmpair.second) {
            cout<<"  " << lmpair.first<<":"<<wfpair.first
            <<", count = " << wfpair.second <<", log-likelihood = "
            << log(wfpair.second/(double)lf) << endl;
            }
        }
        }
        cout<<endl;
    }
};

int main(int argc, char*argv[]) {
    // check command-line arguments
    if(!(argc == 3 || argc == 4)) {
        cout << "Usage: main.exe TRAIN_FILE TEST_FILE [--debug]" << endl;
        return -1;
    }
    else if(argc == 4 && string(argv[3]) != "--debug") {
        cout << "Usage: main.exe TRAIN_FILE TEST_FILE [--debug]" << endl;
        return -2;
    }
    string train_in = argv[1];
    string test_in = argv[2];

    bool debugflag = false;
    if(argc == 4) {
        if(string(argv[3]) == "--debug") {
            debugflag = true;
        }
    }
    try {
        csvstream csv_train_in(train_in);
        csvstream csv_test_in(test_in);

        Classifier classifier;
        classifier.readTrain(csv_train_in,debugflag);

        pair<int,int> openedtest = classifier.readTest(csv_test_in);

        cout<<"performance: " << openedtest.first << " / "
        << openedtest.second << " posts predicted correctly" << endl;  
    }
    catch(csvstream_exception &opentrainerror) {
        cout << opentrainerror.what() << endl;
        return 1;
    }

    return 0;
}
