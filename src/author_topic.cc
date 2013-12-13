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
    for(int i = 0; i < _authors.size(); ++i){
      if(_authors.at(i) == author){
	return i;
      }
    }
    _authors.push_back(author);
    return _authors.size() - 1;
  }

  int set_word(string word){
    for(int i = 0; i < _words.size(); ++i){
      if(_words.at(i) == word){
	return i;
      }
    }
    _words.push_back(word);
    return _words.size() - 1;
  }

  double set_document(vector<string> authors, vector<string> words){
    vector<int> word_ids;
    vector<int> author_ids;
    // set author
    for(vector<string>::iterator i = authors.begin(); i != authors.end(); ++i){
      string author = *i;
      int author_id = set_author(author);
      author_ids.push_back(author_id);
    }
    _author_ids.push_back(author_ids);

    // set word
    for(vector<string>::iterator i = words.begin(); i != words.end(); ++i){
      int word_id = set_word(*i);
      word_ids.push_back(word_id);
    }
    _documents.push_back(word_ids);

    // set random authorss
    vector<int> hidden_authors;
    // set random topics
    vector<int> topics;

    for(int i = 0; i < word_ids.size(); ++i){
      int word_id = word_ids.at(i);
      int init_topic = rand() % this->t;
      int random_author_pos = rand() % author_ids.size();
      int random_author = author_ids.at(random_author_pos);

      // inclement
      ++_c_at[make_pair(random_author, init_topic)];
      ++_c_wt[make_pair(word_id, init_topic)];
      _sum_c_wt[init_topic]++;
      _sum_c_at[random_author]++;
      
      // push
      topics.push_back(init_topic);
      hidden_authors.push_back(random_author);
    }
    _topics.push_back(topics);
    _hidden_authors.push_back(hidden_authors);
  }

  double sampling_prob(int author_id, int word_id, int topic_id){
    int v = _words.size();
    unordered_map<key, int, myhash, myeq>::iterator i;

    int c_wt_count = 0;
    if(_c_wt.find(make_pair(word_id, topic_id)) != _c_wt.end()){
      c_wt_count = _c_wt[make_pair(word_id, topic_id)];
    }
    
    int c_at_count = 0;
    if(_c_at.find(make_pair(author_id, topic_id)) != _c_at.end()){
      c_at_count = _c_at[make_pair(author_id, topic_id)];
    }
    
    double prob = (c_wt_count + this->beta) / (_sum_c_wt[topic_id] + v * this -> beta);
    prob *= (c_at_count + this->alpha) / (_sum_c_at[author_id] + this->t * this -> alpha);

    return prob;
  }

  void sampling(int pos_doc, int pos_word){
    int word_id = (_documents.at(pos_doc)).at(pos_word);
    int prev_topic = (_topics.at(pos_doc)).at(pos_word);
    int prev_author = (_hidden_authors.at(pos_doc)).at(pos_word);
    vector<int> authors = _author_ids.at(pos_doc);

    // vector contains prob density
    // image
    //  |------t_1-----|---t_2---|-t_3-|------t_4-----|
    // 0.0                ^^                         1.0
    vector<double> prob;
    vector<pair<int, int> > topic_author_pairs;

    // declement
    --_c_wt[make_pair(word_id, prev_topic)];
    --_sum_c_wt[prev_topic];

    // sum all topic/author combinations
    for(int j = 0; j < authors.size(); ++j){
      int author_id = authors.at(j);

      // declement temporally
      --_c_at[make_pair(author_id, prev_topic)];
      --_sum_c_at[author_id];

      for(int topic_id = 0; topic_id < this->t; ++topic_id){
	double now_prob = sampling_prob(author_id, word_id, topic_id);
	prob.push_back(now_prob);
	if(prob.size() > 1){
	  prob.at(prob.size() - 1) += prob.at(prob.size() - 2);
	}
	topic_author_pairs.push_back(make_pair(topic_id, author_id));
      }

      // recover
      ++_c_at[make_pair(author_id, prev_topic)];
      ++_sum_c_at[author_id];
    }

    // scaling [0,  1]
    double sum = prob.at(prob.size() - 1);
    for(int i = 0; i < (this->t * authors.size()); ++i){
      prob.at(i) /= sum;
    }
    
    double pos_prob = uniform_rand();
    int new_topic = 0;
    int new_author = 0;
    if(pos_prob > prob.at(0)){
      for(int i = 1; i < (this->t * authors.size()); ++i){
	if((pos_prob <= prob.at(i)) && (pos_prob > prob.at(i - 1))){
	  pair<int, int> new_elem = topic_author_pairs.at(i);
	  new_topic = new_elem.first;
	  new_author = new_elem.second;
	  break;
	}
      }
    }

    // update
    (_topics.at(pos_doc)).at(pos_word) = new_topic;
    (_hidden_authors.at(pos_doc)).at(pos_word) = new_author;

    // declement
    --_c_at[make_pair(prev_author, prev_topic)];
    --_sum_c_at[prev_author];

    // inclement
    ++_c_wt[make_pair(word_id, new_topic)];
    ++_sum_c_wt[new_topic];
    ++_c_at[make_pair(new_author, new_topic)];
    ++_sum_c_at[new_author];
  }

  void sampling_all(){
    boost::progress_display progress( this->loop_count * this->_documents.size() );
    for(int i = 0; i < this->loop_count; ++i){
      for(int pos_doc = 0; pos_doc <  _documents.size(); ++pos_doc){
	for(int pos_word = 1; pos_word < (_documents.at(pos_doc)).size(); ++pos_word){
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
    
    for(int author_id = 0; author_id < _authors.size(); ++author_id){
      // ソート
      vector<pair<double, int> > theta;
      for(int topic_id = 0; topic_id < this->t ; ++topic_id){

	int c_at_count = 0;
	if(_c_at.find(make_pair(author_id, topic_id)) != _c_at.end()){
	  c_at_count = _c_at[make_pair(author_id, topic_id)];
	}
	
	double score = (c_at_count + this->alpha)/(_sum_c_at[author_id] + this->t * this -> alpha);
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
	  ofs_theta << _authors.at(author_id) << "\t" << (*j).second << "\t" << (*j).first << endl;
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

    for(int topic_id = 0; topic_id < this->t; ++topic_id){
      // sort
      vector<pair<double, string> > phi;
      for(int word_id = 0; word_id < _words.size(); ++word_id){
	int c_wt_count = 0;
	if(_c_wt.find(make_pair(word_id, topic_id)) != _c_wt.end()){
	  c_wt_count = _c_wt[make_pair(word_id, topic_id)];
	}
	double score = (c_wt_count + this->beta)/(_sum_c_wt[topic_id] + _words.size() * this -> beta);
	phi.push_back(make_pair(score, _words.at(word_id)));
      }

      // sort all theta
      vector<pair<double, string> > topic_given_theta;
      for(int author_id = 0; author_id < _authors.size(); ++author_id){
	double score = all_theta[make_pair(author_id, topic_id)];
	topic_given_theta.push_back(make_pair(score, _authors.at(author_id)));
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
	  count++;
	}
      }

      sort(topic_given_theta.begin(), topic_given_theta.end());
      count = 0;
      for(j = topic_given_theta.rbegin(); j != topic_given_theta.rend(); ++j){
	if(count >= this->show_limit){
	  break;
	}else{
	  count++;
	}
      }
    }
    ofs_phi.close();
  }
  
private:
  // key: <word_id, topic_id>
  // value: # of assign
  unordered_map<key, int, myhash, myeq> _c_wt;
  
  // key: <author_id, topic_id>
  // value: # of assign
  unordered_map<key, int, myhash, myeq> _c_at;

  // uniq author
  vector<string> _authors;
  // uniq words
  vector<string> _words;

  // vector of author_id, ...
  vector<vector<int> > _author_ids;

  // vector of word_id, ...
  vector<vector<int> > _documents;
  
  // vector of word_topic_id, ...
  vector<vector<int> > _topics;

  // vector of author_id, ...
  vector<vector<int> > _hidden_authors;

  unordered_map<int, int> _sum_c_at;
  unordered_map<int, int> _sum_c_wt;
  
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
    // author:author:author \t w_1:w_2:...
    vector<string> elem = split_string(line, "\t");
    vector<string> authors = split_string(elem.at(0), ":");
    vector<string> words = split_string(elem.at(1), ":");
    t.set_document(authors, words);
  }
  ifs.close();
  t.sampling_all();
  t.output(filename);
}

