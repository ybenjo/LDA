#include "common.cc"
#include "progress.hpp"
using namespace std;

class AuthorTopic{
public:
  AuthorTopic(){
  }

  AuthorTopic(double a, double b, int t, int loop, int show_limit){
    this->alpha = a;
    this->beta = b;
    this->t = t;
    this->loop_count = loop;
    this->show_limit = show_limit;
    srand(time(0));
  }

  int set_author(string author){
    for(int i = 0; i < authors.size(); ++i){
      if(authors.at(i) == author){
	return i;
      }
    }
    authors.push_back(author);
    return authors.size() - 1;
  }

  int set_word(string word){
    for(int i = 0; i < words.size(); ++i){
      if(words.at(i) == word){
	return i;
      }
    }
    words.push_back(word);
    return words.size() - 1;
  }

  double set_document(vector<string> doc){
    vector<int> id_doc;
    // set author
    string author = doc.at(0);
    int author_id = set_author(author);
    id_doc.push_back(author_id);

    // set word
    for(vector<string>::iterator i = doc.begin() + 1; i != doc.end(); ++i){
      int word_id = set_word(*i);
      id_doc.push_back(word_id);
    }
    each_documents.push_back(id_doc);

    // set random topics
    vector<int> topics;
    // dummy topic
    topics.push_back(0);

    for(int i = 1; i < id_doc.size(); ++i){
      int word_id = id_doc.at(i);
      
      sum_c_at[author_id]++;
      
      int init_word_topic = rand() % this->t;
      topics.push_back(init_word_topic);

      c_wt[make_pair(word_id, init_word_topic)]++;
      c_at[make_pair(author_id, init_word_topic)]++;
      sum_c_wt[init_word_topic]++;
    }
    each_topics.push_back(topics);
  }

  double sampling_prob(int author_id, int word_id, int topic_id){
    int v = words.size();
    unordered_map<key, int, myhash, myeq>::iterator i;

    int c_wt_count = 0;
    if(c_wt.find(make_pair(word_id, topic_id)) != c_wt.end()){
      c_wt_count = c_wt[make_pair(word_id, topic_id)];
    }
    
    int c_at_count = 0;
    if(c_at.find(make_pair(author_id, topic_id)) != c_at.end()){
      c_at_count = c_at[make_pair(author_id, topic_id)];
    }
    
    double prob = (c_wt_count + this->beta) / (sum_c_wt[topic_id] + v * this -> beta);
    prob *= (c_at_count + this->alpha) / (sum_c_at[author_id] + this->t * this -> alpha);

    return prob;
  }

  void sampling(int pos_doc, int pos_word){
    int author_id = (each_documents.at(pos_doc)).at(0);
    int word_id = (each_documents.at(pos_doc)).at(pos_word);
    
    // except current status
    int prev_word_topic = (each_topics.at(pos_doc)).at(pos_word);
    c_wt[make_pair(word_id, prev_word_topic)]--;
    c_at[make_pair(author_id, prev_word_topic)]--;
    sum_c_wt[prev_word_topic]--;

    // vector contains prob density
    // image
    //  |------t_1-----|---t_2---|-t_3-|------t_4-----|
    // 0.0                ^^                         1.0
    vector<double> prob;
    // sum all topics
    for(int i = 0; i < this->t; ++i){
      double now_prob = sampling_prob(author_id, word_id, i);
      prob.push_back(now_prob);
      if(i > 0){
	prob.at(i) += prob.at(i - 1);
      }
    }
    // scaling [0,  1]
    double sum = prob.at(prob.size() - 1);
    for(int i = 0; i < this->t; ++i){
      prob.at(i) /= sum;
    }
    
    double pos_prob = uniform_rand();
    int new_topic = 0;
    if(pos_prob > prob.at(0)){
      for(int i = 1; i < this->t; ++i){
	if((pos_prob <= prob.at(i)) && (pos_prob > prob.at(i - 1))){
	  new_topic = i;
	  break;
	}
      }
    }

    // update each val
    (each_topics.at(pos_doc)).at(pos_word) = new_topic;
    c_at[make_pair(author_id, new_topic)]++;
    c_wt[make_pair(word_id, new_topic)]++;

    // update too
    sum_c_wt[new_topic]++;
  }

  void sampling_all(){
    boost::progress_display progress( this->loop_count * this->each_documents.size() );
    for(int i = 0; i < this->loop_count; ++i){
      for(int pos_doc = 0; pos_doc <  each_documents.size(); ++pos_doc){
	for(int pos_word = 1; pos_word < (each_documents.at(pos_doc)).size(); ++pos_word){
	  sampling(pos_doc, pos_word);
	}
	++progress;
      }
    }
  }

  void output(char* filename){
    // output theta
    ostringstream oss_theta;
    oss_theta << filename << "_theta" ;
    ofstream ofs_theta;
    ofs_theta.open((oss_theta.str()).c_str());

    // key: <author_id, topic_id>
    // value: theta
    unordered_map<key, double, myhash, myeq> all_theta;
    
    for(int author_id = 0; author_id < authors.size(); ++author_id){
      // ソート
      vector<pair<double, int> > theta;
      for(int topic_id = 0; topic_id < this->t ; ++topic_id){

	int c_at_count = 0;
	if(c_at.find(make_pair(author_id, topic_id)) != c_at.end()){
	  c_at_count = c_at[make_pair(author_id, topic_id)];
	}
	
	double score = (c_at_count + this->alpha)/(sum_c_at[author_id] + this->t * this -> alpha);
	theta.push_back(make_pair(score, topic_id));
	all_theta[make_pair(author_id, topic_id)] = score;
      }

      // output
      sort(theta.begin(), theta.end());
      vector<pair<double, int> >::reverse_iterator j;
      int count = 0;
      for(j = theta.rbegin(); j != theta.rend(); ++j){
	if(count >= this->show_limit){
	  break;
	}else{
	  ofs_theta << authors.at(author_id) << "\t" << (*j).second << "\t" << (*j).first << endl;
	  count++;
	}
      }
    }
    ofs_theta.close();

    // output phi
    ostringstream oss_phi;
    oss_phi << filename << "_phi" ;
    ofstream ofs_phi;
    ofs_phi.open((oss_phi.str()).c_str());

    // output mix
    ostringstream oss_mix;
    oss_mix << filename << "_mix" ;
    ofstream ofs_mix;
    ofs_mix.open((oss_mix.str()).c_str());

    for(int topic_id = 0; topic_id < this->t; ++topic_id){
      // sort
      vector<pair<double, string> > phi;
      for(int word_id = 0; word_id < words.size(); ++word_id){
	int c_wt_count = 0;
	if(c_wt.find(make_pair(word_id, topic_id)) != c_wt.end()){
	  c_wt_count = c_wt[make_pair(word_id, topic_id)];
	}
	double score = (c_wt_count + this->beta)/(sum_c_wt[topic_id] + words.size() * this -> beta);
	phi.push_back(make_pair(score, words.at(word_id)));
      }

      // sort all theta
      vector<pair<double, string> > topic_given_theta;
      for(int author_id = 0; author_id < authors.size(); ++author_id){
	double score = all_theta[make_pair(author_id, topic_id)];
	topic_given_theta.push_back(make_pair(score, authors.at(author_id)));
      }

      // output
      sort(phi.begin(), phi.end());
      vector<pair<double, string> >::reverse_iterator j;
      int count = 0;
      for(j = phi.rbegin(); j != phi.rend(); ++j){
	if(count >= this->show_limit){
	  break;
	}else{
	  ofs_phi << topic_id << "\t" << (*j).second << "\t" << (*j).first << endl;
	  ofs_mix << topic_id << "\t" << (*j).second << "\t" << (*j).first << endl;
	  count++;
	}
      }

      ofs_mix << "----------" << endl;
      sort(topic_given_theta.begin(), topic_given_theta.end());
      count = 0;
      for(j = topic_given_theta.rbegin(); j != topic_given_theta.rend(); ++j){
	if(count >= this->show_limit){
	  break;
	}else{
	  ofs_mix << topic_id << "\t" << (*j).second << "\t" << (*j).first << endl;
	  count++;
	}
      }
      ofs_mix << "------------------------------" << endl;
    }
    ofs_phi.close();
    ofs_mix.close();
  }
  
private:
  // key: <word_id, topic_id>
  // value: # of assign
  unordered_map<key, int, myhash, myeq> c_wt;
  
  // key: <author_id, topic_id>
  // value: # of assign
  unordered_map<key, int, myhash, myeq> c_at;

  // uniq author
  vector<string> authors;
  // uniq words
  vector<string> words;
  
  // vector of author_id, word_id, ...
  // each element: document
  vector<vector<int> > each_documents;
  
  // vector of dummy_topic_id, word_topic_id, ...
  // each element: document
  vector<vector<int> > each_topics;

  unordered_map<int, int> sum_c_at;
  unordered_map<int, int> sum_c_wt;
  
  // params
  double alpha;
  double beta;
  int t;
  int loop_count;
  int show_limit;
};

int main(int argc, char** argv){
  char *filename = argv[1];
  int topic_size = atoi(argv[3]);
  double alpha = atof(argv[2]);
  double beta = 0.01;
  AuthorTopic t(alpha, beta, topic_size, atoi(argv[4]), atoi(argv[5]));
  
  ifstream ifs;
  ifs.open(filename, ios::in);
  string line;
  while(getline(ifs, line)){
    // file format
    // author \t w_1 \t w_2 ...
    vector<string> elem = split_string(line, "\t");
    t.set_document(elem);
  }
  ifs.close();
  t.sampling_all();
  t.output(filename);
}
