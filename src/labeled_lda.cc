#include "common.cc"
#include <boost/random.hpp>
#include "progress.hpp"

using namespace std;

// ref http://d.hatena.ne.jp/n_shuyo/20100407/random
template<class D, class G = boost::mt19937>
class DICE {
  G gen_;
  D dst_;
  boost::variate_generator<G, D> rand_;
public:
  DICE() : gen_(static_cast<unsigned long>(1)), rand_(gen_, dst_) {}
  template<typename T1>
  DICE(T1 a1) : gen_(static_cast<unsigned long>(1)), dst_(a1), rand_(gen_, dst_) {}
  template<typename T1, typename T2>
  DICE(T1 a1, T2 a2) : gen_(static_cast<unsigned long>(1)), dst_(a1, a2), rand_(gen_, dst_) {}

  typename D::result_type operator()() { return rand_(); }
};

class LabeledLDA{
public:
  LabeledLDA(){
  }

  LabeledLDA(double a, double b, int loop, int show_limit){
    _alpha = a;
    _beta = b;
    _loop_count = loop;
    _show_limit = show_limit;

    DICE<boost::uniform_real<> > rand(0, 1);
    _rand = rand;
  }

  int set_document_name(string document){
    for(int i = 0; i < _documents.size(); ++i){
      if(_documents.at(i) == document){
	return i;
      }
    }
    _documents.push_back(document);
    return _documents.size() - 1;
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

  int set_label(string label){
    for(int i = 0; i < _labels.size(); ++i){
      if(_labels.at(i) == label){
	return i;
      }
    }
    _labels.push_back(label);
    return _labels.size() - 1;
  }

  void set_label_information(vector<string> elem){
    // 第一要素は doc_name
    // それ以降にラベル
    string doc_name = elem.at(0);
    int document_id = set_document_name(doc_name);
    vector<int> labels;

    // label を id に変換して格納
    for(int i = 1; i < elem.size(); ++i){
      int label_id = set_label(elem.at(i));
      labels.push_back(label_id);
    }
    _doc_labels[document_id] = labels;
  }

  void set_garbage_label(){
    string garbage_label = "___GARBAGE_LABELS__";
    for(int doc_id = 0; doc_id < _documents.size(); ++doc_id){
      // 全文書共通のゴミラベルを追加する
      vector<int> new_labels = _doc_labels[doc_id];
      int garbage_label_id = set_label(garbage_label);
      new_labels.push_back(garbage_label_id);
      _doc_labels[doc_id] = new_labels;
    }
  }

  double set_document(vector<string> doc){
    vector<int> id_doc;
    // set document
    string document_name = doc.at(0);
    int document_id = set_document_name(document_name);
    id_doc.push_back(document_id);

    // set word
    for(vector<string>::iterator i = doc.begin() + 1; i != doc.end(); ++i){
      int word_id = set_word(*i);
      id_doc.push_back(word_id);
    }
    _each_documents.push_back(id_doc);

    // set random topics
    vector<int> topics;
    // dummy topic
    // なんでここで dummy を入れているかというと，
    // document word word word の形式なのでそのまま扱うと word_pos がひとつずれる
    // よって先頭に0のトピックを入れ，スキップして揃えている
    topics.push_back(0);

    for(int i = 1; i < id_doc.size(); ++i){
      int word_id = id_doc.at(i);
      
      _sum_c_at[document_id]++;

      // 問題はここで，初期トピックの時点で手元のトピックから振らなければならない
      // なので，毎 doc ごとに新しく label の個数だけ面を持つサイコロを作り直す
      int label_size = (_doc_labels[document_id]).size();
      DICE<boost::uniform_int<> > dice(0, (label_size - 1));
      int random_index = dice();
      int init_word_topic = _doc_labels[document_id].at(random_index);
      topics.push_back(init_word_topic);

      _c_wt[make_pair(word_id, init_word_topic)]++;
      _c_at[make_pair(document_id, init_word_topic)]++;
      _sum_c_wt[init_word_topic]++;
    }
    _each_topics.push_back(topics);
  }

  double sampling_prob(int document_id, int word_id, int topic_id){
    int v = _words.size();
    unordered_map<key, int, myhash, myeq>::iterator i;

    int c_wt_count = 0;
    if(_c_wt.find(make_pair(word_id, topic_id)) != _c_wt.end()){
      c_wt_count = _c_wt[make_pair(word_id, topic_id)];
    }
    
    int c_at_count = 0;
    if(_c_at.find(make_pair(document_id, topic_id)) != _c_at.end()){
      c_at_count = _c_at[make_pair(document_id, topic_id)];
    }
    
    double prob = (c_wt_count + _beta) / (_sum_c_wt[topic_id] + v * _beta);
    // ここが曖昧
    // LDA だと _t * _alpha になっているけど ???
    prob *= (c_at_count + _alpha) / (_sum_c_at[document_id] + (_doc_labels[document_id]).size() * _alpha);

    return prob;
  }

  void sampling(int pos_doc, int pos_word){
    int document_id = (_each_documents.at(pos_doc)).at(0);
    int word_id = (_each_documents.at(pos_doc)).at(pos_word);
    
    // except current status
    int prev_word_topic = (_each_topics.at(pos_doc)).at(pos_word);
    _c_wt[make_pair(word_id, prev_word_topic)]--;
    _c_at[make_pair(document_id, prev_word_topic)]--;
    _sum_c_wt[prev_word_topic]--;

    // vector contains prob density
    // image
    //  |------t_1-----|---t_2---|-t_3-|------t_4-----|
    // 0.0                ^^                         1.0
    vector<double> prob;
    // ここで全てのトピックで計算せずに， doc に振られた label の数だけサンプリングする
    vector<int> now_labels = _doc_labels[document_id];
    int label_size = now_labels.size();
    
    for(int i = 0; i < label_size; ++i){
      int label_id = now_labels.at(i);
      double now_prob = sampling_prob(document_id, word_id, label_id);
      prob.push_back(now_prob);
      if(i > 0){
	prob.at(i) += prob.at(i - 1);
      }
    }

    // scaling [0,  1]
    double sum = prob.at(prob.size() - 1);
    for(int i = 0; i < label_size; ++i){
      prob.at(i) /= sum;
    }
    
    double pos_prob = _rand();
    int new_topic = now_labels.at(0);
    if(pos_prob > prob.at(0)){
      for(int i = 1; i < label_size; ++i){
	if((pos_prob <= prob.at(i)) && (pos_prob > prob.at(i - 1))){
	  // i ではなく pos を参照する
	  new_topic = now_labels.at(i);
	  break;
	}
      }
    }

    // update each val
    (_each_topics.at(pos_doc)).at(pos_word) = new_topic;
    _c_at[make_pair(document_id, new_topic)]++;
    _c_wt[make_pair(word_id, new_topic)]++;

    // update too
    _sum_c_wt[new_topic]++;
  }

  void sampling_all(){
    for(int i = 0; i < _loop_count; ++i){
      for(int pos_doc = 0; pos_doc <  _each_documents.size(); ++pos_doc){
	for(int pos_word = 1; pos_word < (_each_documents.at(pos_doc)).size(); ++pos_word){
	  sampling(pos_doc, pos_word);
	}
      }
    }
  }

  void output(char* filename){
    // output theta
    ostringstream oss_theta;
    oss_theta << filename << "_theta" ;
    ofstream ofs_theta;
    ofs_theta.open((oss_theta.str()).c_str());

    // key: <document_id, topic_id>
    // value: theta
    unordered_map<key, double, myhash, myeq> all_theta;
    
    for(int document_id = 0; document_id < _documents.size(); ++document_id){
      // ソート
      vector<pair<double, int> > theta;
      vector<int> labels = _doc_labels[document_id];
      int label_size = labels.size();
      for(int topic_pos = 0; topic_pos < label_size ; ++topic_pos){
	int topic_id = labels.at(topic_pos);
	int c_at_count = 0;
	if(_c_at.find(make_pair(document_id, topic_id)) != _c_at.end()){
	  c_at_count = _c_at[make_pair(document_id, topic_id)];
	}

	// ここもあやふや
	// double score = (c_at_count + _alpha)/(_sum_c_at[document_id] + _labels.size() * _alpha);
	double score = (c_at_count + _alpha)/(_sum_c_at[document_id] + (_doc_labels[document_id]).size() * _alpha);
	theta.push_back(make_pair(score, topic_id));
	all_theta[make_pair(document_id, topic_id)] = score;
      }

      // output
      sort(theta.begin(), theta.end());
      vector<pair<double, int> >::reverse_iterator j;
      int count = 0;
      for(j = theta.rbegin(); j != theta.rend(); ++j){
	if(count >= _show_limit){
	  break;
	}else{
	  // ofs_theta << documents.at(document_id) << "\t" << (*j).second << "\t" << (*j).first << endl;
	  int label_pos = (*j).second;
	  string label_name = _labels.at(label_pos);
	  ofs_theta << _documents.at(document_id) << "\t" << label_name << "\t" << (*j).first << endl;
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

    int all_label_size = _labels.size();
    for(int topic_id = 0; topic_id < all_label_size; ++topic_id){
      // sort
      vector<pair<double, string> > phi;
      for(int word_id = 0; word_id < _words.size(); ++word_id){
	int c_wt_count = 0;
	if(_c_wt.find(make_pair(word_id, topic_id)) != _c_wt.end()){
	  c_wt_count = _c_wt[make_pair(word_id, topic_id)];
	}
	double score = (c_wt_count + _beta)/(_sum_c_wt[topic_id] + _words.size() * _beta);
	phi.push_back(make_pair(score, _words.at(word_id)));
      }

      // sort all theta
      vector<pair<double, string> > topic_given_theta;
      for(int document_id = 0; document_id < _documents.size(); ++document_id){
	double score = all_theta[make_pair(document_id, topic_id)];
	topic_given_theta.push_back(make_pair(score, _documents.at(document_id)));
      }

      // output
      sort(phi.begin(), phi.end());
      vector<pair<double, string> >::reverse_iterator j;
      int count = 0;
      for(j = phi.rbegin(); j != phi.rend(); ++j){
	if(count >= _show_limit){
	  break;
	}else{
	  // ofs_phi << topic_id << "\t" << (*j).second << "\t" << (*j).first << endl;
	  string label_name = _labels.at(topic_id);
	  ofs_phi << label_name << "\t" << (*j).second << "\t" << (*j).first << endl;
	  count++;
	}
      }

      sort(topic_given_theta.begin(), topic_given_theta.end());
      count = 0;
      for(j = topic_given_theta.rbegin(); j != topic_given_theta.rend(); ++j){
	if(count >= _show_limit){
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
  
  // key: <document_id, topic_id>
  // value: # of assign
  unordered_map<key, int, myhash, myeq> _c_at;

  // uniq document
  vector<string> _documents;
  // uniq words
  vector<string> _words;
  // uniq labels
  vector<string> _labels;

  // key: doc_name_id
  // value: labels
  unordered_map<int, vector<int> > _doc_labels;
  
  // vector of document_id, word_id, ...
  // each element: document
  vector<vector<int> > _each_documents;
  
  // vector of dummy_topic_id, word_topic_id, ...
  // each element: document
  vector<vector<int> > _each_topics;

  unordered_map<int, int> _sum_c_at;
  unordered_map<int, int> _sum_c_wt;
  
  // params
  double _alpha;
  double _beta;
  int _loop_count;
  int _show_limit;

  // rand
  DICE<boost::uniform_real<> > _rand;
};

int main(int argc, char** argv){
  char *filename = argv[1];
  char *label_filename = argv[2];
  double alpha = atof(argv[3]);
  double beta = 0.01;
  int loop = atoi(argv[4]);
  int show_lim = atoi(argv[5]);
  LabeledLDA t(alpha, beta, loop, show_lim);

  ifstream ifs;
  string line;
  // read doc - label
  // ドキュメントの初期化にラベル情報が必要なので先に読む
  ifs.open(label_filename, ios::in);
  while(getline(ifs, line)){
    // file format
    // doc_name \t w_1 \t w_2 ...
    vector<string> elem = split_string(line, "\t");
    t.set_label_information(elem);
  }
  ifs.close();
  // ゴミラベル追加
  t.set_garbage_label();

  // read doc - word
  ifs.open(filename, ios::in);
  while(getline(ifs, line)){
    // file format
    // doc_name \t w_1 \t w_2 ...
    vector<string> elem = split_string(line, "\t");
    t.set_document(elem);
  }
  ifs.close();

  t.sampling_all();
  t.output(filename);
}
