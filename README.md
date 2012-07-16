# About
Practice of LDA  

# todo  
- Normal LDA.

## Implementation
- Michal Rosen-Zvi, Tom Griffiths, Mark Steyvers, Padhraic Smyth. The Author-Topic Model for Authors and Documents. UAI (2004)  
  
## usage

    make author_topic
    ./author_topic input.tsv alpha t iter lim
    
## params
- input.tsv: input file. Each line is document.  
  - format: author \t word \t word ...  
- alpha: \alpha.  
  - \beta if fix(= 0.01)  
- t: # of topics.  
- iter: # of sampling iterations.  
- lim: limit of outputs.  
