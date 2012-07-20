# About
Practice of LDA and other Topic Model based Collapsed Gibbs Sampling.

# todo  
- Normal LDA  
- Dynamic Topic Model   

## Implementation
- Michal Rosen-Zvi, Tom Griffiths, Mark Steyvers, Padhraic Smyth. The Author-Topic Model for Authors and Documents. UAI (2004)  
- Yi Fang, Luo Si, Naveen Somasundaram, Zhengtao Yu. Mining contrastive opinions on political texts using cross-perspective topic model. WSDM (2012)    

## Requirements
- g++ ( >= 4.6)

## Author-Topic Model
### usage

    make author_topic
    ./author_topic input.tsv alpha t iter lim
    
### params
- input.tsv: input file. Each line is document.  
  - format: author \t word \t word ...  
- alpha: \alpha.  
  - \beta if fix(= 0.01)  
- t: # of topics.  
- iter: # of sampling iterations.  
- lim: limit of outputs.   

## Cross-Perspective Topic Model
### usage

    make cpt
    ./cpt per1_topic.tsv per1_opinion.tsv per2_topic.tsv per2_opinion.tsv alpha beta beta1 beta2 k iter lim
    
### params
- per1\_topic, per2\_topic: input topic file. Each line is document.  
  - format: word \t word ...  
- per1\_opinion, per2\_opinion: input opinion file. Each line is document.  
  - format: word \t word ...  
  - topic/opinion file must same size.
- alpha: \alpha.  
- beta, beta1, beta2: \beta.   
- k: # of topics.  
- iter: # of sampling iterations.  
- lim: limit of outputs.  
