#include "common.cc"
using namespace std;
class CPT{
public:
  CPT(){
  }

  CPT(double a, double b, double b_1, double b_2, int k, int iter){
    srand((unsigned) time(NULL));
    this->alpha = a;
    this->beta = b;
    this->perspective_beta[1] = b_1;
    this->perspective_beta[2] = b_2;
    this->k = k;
    this->iter = iter;
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
  
  int set_opinion_word(string word){
    for(int i = 0; i < opinion_words.size(); ++i){
      if(opinion_words.at(i) == word){
	return i;
      }
    }
    opinion_words.push_back(word);
    return opinion_words.size() - 1;
  }

  void set_document_opinion(vector<string> line){
    vector<int> ops, ops_topic;
    for(vector<string>::iterator i = line.begin(); i != line.end(); ++i){
      int word_id = set_opinion_word(*i);
      ops.push_back(word_id);
      
      // initialize random topic
      int init_opinion_topic = rand() % this->k;
      
      ops_topic.push_back(init_opinion_topic);
      n_rs[make_pair(word_id, init_opinion_topic)]++;
      sum_n_rs[init_opinion_topic]++;
    }
    doc_opinion.push_back(ops);
    doc_opinion_topic.push_back(ops_topic);
  }

  void set_document(vector<string> line, int perspective){
    int doc_id = doc_word.size();
    vector<int> ws, ws_topic;

    doc_perspective[doc_id] = perspective;
    
    for(vector<string>::iterator i = line.begin(); i != line.end(); ++i){

      int word_id = set_word(*i);
      ws.push_back(word_id);

      // initialize random topic
      int init_word_topic = rand() % this->k;
      
      ws_topic.push_back(init_word_topic);
      n_kd[make_pair(init_word_topic, doc_id)]++;
      n_vk[make_pair(word_id, init_word_topic)]++;
      n_sd[make_pair(init_word_topic, doc_id)]++;
      
      n_td[doc_id]++;
      sum_n_kd[doc_id]++;
      sum_n_vk[init_word_topic]++;
    }
    doc_word.push_back(ws);
    doc_word_topic.push_back(ws_topic);
  }

  // before call this, except n_kd, n_vk, sum_n_vk.
  double prob_topic(int doc_id, int word_id, int topic_id){
    double ret = (n_kd[make_pair(topic_id, doc_id)] + alpha) / (sum_n_kd[doc_id] + k * alpha);
    return ret * (n_vk[make_pair(word_id, topic_id)] + beta) / (sum_n_vk[topic_id] + words.size() * beta);
  }

  // before call this, except sum_n_rs
  double prob_opinion(int doc_id, int opinion_id, int topic_id){
    int perspective = doc_perspective[doc_id];
    double per_beta = perspective_beta[perspective];
    double ret = (n_rs[make_pair(opinion_id, topic_id)] * per_beta) / (sum_n_rs[topic_id] +  opinion_words.size() * per_beta);
    return ret * n_sd[make_pair(topic_id, doc_id)] / n_td[doc_id];
  }

  void sampling_topic(int doc_id, int pos_word){
    int word_id = (doc_word.at(doc_id)).at(pos_word);
    int prev_topic = (doc_word_topic.at(doc_id)).at(pos_word);
    
    // except current status
    // n_kd, n_vk, sum_n_vk
    n_kd[make_pair(prev_topic, doc_id)]--;
    n_vk[make_pair(word_id, prev_topic)]--;
    sum_n_vk[prev_topic]--;

    // Not exist in sampling equation, but need this for opinion sampling.
    n_sd[make_pair(prev_topic, doc_id)]--;
    
    // sampling prob
    vector<double> prob;
    prob.push_back(prob_topic(doc_id, word_id, 0));
    for(int i = 1; i < this->k; ++i){
      // adding
      double val = prob_topic(doc_id, word_id, i);
      prob.push_back(val + prob.at(i - 1));
    }
    // Normalize [0, 1]
    for(int i = 0; i < this->k; ++i){
      prob.at(i) /= prob.at(prob.size() - 1);
    }

    double pos_topic = uniform_rand();
    int new_topic = 0;
    if(pos_topic > prob.at(0)){
      for(int i = 1; i < this->k; ++i){
        if((pos_topic <= prob.at(i)) && (pos_topic > prob.at(i - 1))){
          new_topic = i;
          break;
        }
      }
    }

    // update each variables
    (doc_word_topic.at(doc_id)).at(pos_word) = new_topic;
    n_kd[make_pair(new_topic, doc_id)]++;
    n_vk[make_pair(word_id, new_topic)]++;
    sum_n_vk[new_topic]++;

    // Not exist in sampling equation, but need this for opinion sampling.
    n_sd[make_pair(prev_topic, doc_id)]++;
  }

  void sampling_opinion(int doc_id, int pos_op){
    int op_id = (doc_opinion.at(doc_id)).at(pos_op);
    int prev_topic = (doc_opinion_topic.at(doc_id)).at(pos_op);
    
    // except current status
    n_rs[make_pair(op_id, prev_topic)]--;
    
    // sampling prob
    vector<double> prob;
    prob.push_back(prob_opinion(doc_id, op_id, 0));
    for(int i = 1; i < this->k; ++i){
      // adding
      double val = prob_opinion(doc_id, op_id, i);
      prob.push_back(val + prob.at(i - 1));
    }
    // Normalize [0, 1]
    for(int i = 0; i < this->k; ++i){
      prob.at(i) /= prob.at(prob.size() - 1);
    }

    double pos_topic = uniform_rand();
    int new_topic = 0;
    if(pos_topic > prob.at(0)){
      for(int i = 1; i < this->k; ++i){
        if((pos_topic <= prob.at(i)) && (pos_topic > prob.at(i - 1))){
          new_topic = i;
          break;
        }
      }
    }

    // update each variables
    (doc_opinion_topic.at(doc_id)).at(pos_op) = new_topic;
    n_rs[make_pair(op_id, new_topic)]--;
  }

  void sampling_all(){

    for(int i = 0; i < this->iter; ++i){
      // All topics
      for(int doc_id = 0; doc_id < doc_word.size(); ++doc_id){
	vector<int> doc = doc_word.at(doc_id);
	for(int pos_word = 0; pos_word < doc.size(); ++pos_word){
	  sampling_topic(doc_id, pos_word);
	}
      }

      // All opinions
      for(int doc_id = 0; doc_id < doc_opinion.size(); ++doc_id){
	vector<int> doc = doc_opinion.at(doc_id);
	for(int pos_op = 0; pos_op < doc.size(); ++pos_op){
	  sampling_opinion(doc_id, pos_op);
	}
      }
    }
  }

  void output(int limit, char* file_1, char* file_2){
    // Write perspective common topic-word topic distribution (\phi_{vk})
    ostringstream oss_c_phi;
    ofstream ofs_c_phi;
    oss_c_phi << file_1 << "_common" ;
    ofs_c_phi.open((oss_c_phi.str()).c_str());
    for(int topic_id = 0; topic_id < this->k; ++topic_id){
      vector<pair<double, int> > tmp;
      
      for(int word_id = 0; word_id < words.size(); ++word_id){
	double theta = (n_vk[make_pair(word_id, topic_id)] + this->beta) / (sum_n_vk[topic_id] + words.size() * this->beta);
	tmp.push_back(make_pair(theta, word_id));
      }
      
      sort(tmp.begin(), tmp.end());
      int count = 0;
      vector<pair<double, int> >::reverse_iterator i; 
      for(i = tmp.rbegin(); i != tmp.rend(); ++i){
	if(count >= limit){
	  break;
	}else{
	  pair<double, int> e = *i;
	  string word = words.at(e.second);
	  ofs_c_phi << topic_id << "," << word << "," << e.first << endl;
	  count++;
	}
      }
    }
    
    // Write perspective specific opinion-word topic distribution (\phi_{o, rs}^{i})
    for(int per_id = 1; per_id <= 2; ++per_id){
      ostringstream oss_s_phi;
      ofstream ofs_s_phi;
      
      if(per_id == 1){
	oss_s_phi << file_1 << "_specific";
      }else{
	oss_s_phi << file_2 << "_specific";
      }

      // count up n_{o, rs}
      // Maybe n_{o, rs} means n_{rs} in perspective_i
      unordered_map<key, int, myhash, myeq> n_o_rs;
      unordered_map<int, int> sum_n_o_rs;

      for(int doc_id = 0; doc_id < doc_opinion_topic.size(); ++doc_id){
	if(doc_perspective[doc_id] == per_id){
	  vector<int> doc = doc_opinion_topic.at(doc_id);
	  for(int pos_op = 0; pos_op < doc.size(); ++pos_op){
	    int topic_id = doc.at(pos_op);
	    int op_id = (doc_opinion.at(doc_id)).at(pos_op);
	    n_o_rs[make_pair(op_id, topic_id)]++;
	    sum_n_o_rs[topic_id]++;
	  }
	}
      }
      
      ofs_s_phi.open((oss_s_phi.str()).c_str());
      
      for(int topic_id = 0; topic_id < this->k; ++topic_id){
	vector<pair<double, int> > tmp;
	for(int word_id = 0; word_id < opinion_words.size(); ++word_id){
	  double theta = (n_o_rs[make_pair(word_id, topic_id)] + perspective_beta[per_id]);
	  theta  /= (sum_n_o_rs[topic_id] + opinion_words.size() * perspective_beta[per_id]);

	  tmp.push_back(make_pair(theta, word_id));
	}
	
	sort(tmp.begin(), tmp.end());
	vector<pair<double, int> >::reverse_iterator i; 
	int count = 0;
	for(i = tmp.rbegin(); i != tmp.rend(); ++i){
	  if(count >= limit){
	    break;
	  }else{
	    pair<double, int> e = *i;
	    string opinion_word = opinion_words.at(e.second);
	    ofs_s_phi << topic_id << "," << opinion_word << "," << e.first << endl;
	    count++;
	  }
	}
      }
    }
  }
    
private:
  vector<string> words;
  vector<string> opinion_words;
  vector<vector<int> > doc_word;
  vector<vector<int> > doc_opinion;
  unordered_map<int, int> doc_perspective;

  // latent variables
  vector<vector<int> > doc_word_topic;
  vector<vector<int> > doc_opinion_topic;
  unordered_map<int, int> n_td;
  unordered_map<key, int, myhash, myeq> n_kd;
  unordered_map<key, int, myhash, myeq> n_vk;
  unordered_map<key, int, myhash, myeq> n_rs;
  unordered_map<key, int, myhash, myeq> n_sd;

  // sum of latent var
  unordered_map<int, int> sum_n_kd;
  unordered_map<int, int> sum_n_vk;
  unordered_map<int, int> sum_n_rs;
  
  // params
  double alpha;
  double beta;
  unordered_map<int, double> perspective_beta;
  int k;
  int iter;
};

int main(int argc, char** argv){

  double alpha = atof(argv[5]);
  double beta = atof(argv[6]);
  double beta_1 = atof(argv[7]);
  double beta_2 = atof(argv[8]);
  double k = atoi(argv[9]);
  double iter = atoi(argv[10]);
  double limit = atoi(argv[11]);
  
  CPT c(alpha, beta, beta_1, beta_2, k, iter);
  ifstream ifs;
  string line;

  // read topic in perspective 1
  char* file_1_topic = argv[1];
  ifs.open(file_1_topic, ios::in);
  while(getline(ifs, line)){
    // file format
    // w_1 \t w_2 ... 
    vector<string> elem = split_string(line, "\t");
    c.set_document(elem, 1);
  }
  ifs.close();

  // read opinion in perspective 1
  char* file_1_opinion = argv[2];
  ifs.open(file_1_opinion, ios::in);
  while(getline(ifs, line)){
    // file format
    // o_1 \t o_2 ... 
    vector<string> elem = split_string(line, "\t");
    c.set_document_opinion(elem);
  }
  ifs.close();

  // read topic in perspective 2
  char* file_2_topic = argv[3];
  ifs.open(file_2_topic, ios::in);
  while(getline(ifs, line)){
    // file format
    // w_1 \t w_2 ... 
    vector<string> elem = split_string(line, "\t");
    c.set_document(elem, 2);
  }
  ifs.close();

  // read opinion in perspective 2
  char* file_2_opinion = argv[4];
  ifs.open(file_2_opinion, ios::in);
  while(getline(ifs, line)){
    // file format
    // o_1 \t o_2 ... 
    vector<string> elem = split_string(line, "\t");
    c.set_document_opinion(elem);
  }
  ifs.close();
 
  c.sampling_all();
  c.output(limit, file_1_topic, file_2_topic);
}
