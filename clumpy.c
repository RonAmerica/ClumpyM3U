/*

	Clumpy

	Makes your m3u playlists clumpy

	The program loads a playlist, reorders it so songs with
	the same words in the names of the files are sequential,
	and outputs the new playlist.

	By W R "Ron" Spain Jr

	See https://github.com/RonAmerica/ClumpyM3U

*/


#pragma GCC optimize("Os")


#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>
#include<time.h>



typedef struct{
 char*s;
 uint32_t*w;
}Entry;


typedef struct{
 uint32_t w;
 uint32_t c;
 float sc;
}Word;


typedef struct{
 char*infile;
 char*outfile;
 unsigned timeLimit;
 unsigned loMut;
 unsigned hiMut;
 unsigned bufSize;
 bool shuffle;
 bool verbose;
}Opt;


Opt opt={
 .infile="playlist.m3u",
 .outfile="clumpy.m3u",
 .timeLimit=10,
 .loMut=1,
 .hiMut=2,
 .bufSize=1024,
 .shuffle=8,
 .verbose=1,
};


char*helpText=
	"\n"
	"\tClumpy, The Amazing M3U Playlist Clumpifier\n"
	"\n"
	"Options:\n"
	"\n"
	"-i	infile\n"
	"-o	outfile\n"
	"-t	time in seconds to run, default 10, more=clumpier\n"
	"-ml	low number of mutations, default 1\n"
	"-mh	high number of mutations, default 2\n"
	"-b	buffer size, default 1024, increase for longer names\n"
	"-r	shuffle amount, default 8, prevents recurring order\n"
	"-v	verbosity, 1 or 0\n"
	"-h	show this help\n";



Entry*entry=0;
Entry*bestEntry=0;
Word*word=0;

float score=0;
float bestScore=0;

unsigned words=0;
unsigned entries=0;
unsigned mutations=0;
unsigned entryMems=0;


__attribute__((cold))
void die(char*s){
 fflush(stdout);
 fputs("\n=== ERROR ===\n",stderr);
 perror(s);
 exit(1);
}


__attribute__((cold))
FILE*fop(char*a,char*b){
 FILE*f=0;
 if(strcmp(a,"-"))f=fopen(a,b);
 else{
  switch(*b){
   case 'r':
   f=stdin;
   break;
   case 'w':
   f=stdout;
   break;
   default:
   die("Odd file options");
  }
 }
 if(!f)die(a);
 return f;
}


__attribute__((cold))
void rtrim(char*s){
 char*p=s+strlen(s);
 while(1){
  --p;
  if(p>=s&&isspace(*p))*p=0;
  else break;
 }
}


__attribute__((cold))
void*real(void*m,unsigned n){
 if(!n)die("Zero size realloc");
 m=realloc(m,n);
 if(!m)die("realloc");
 return m;
}


__attribute__((cold))
void*mal(unsigned n){
 return real(0,n);
}


__attribute__((cold))
unsigned flines(char*name){
 char*buf;
 char*r;
 unsigned n=0;
 FILE*f;
 buf=mal(opt.bufSize);
 f=fop(name,"rt");
 while(!feof(f)){
  r=fgets(buf,opt.bufSize,f);
  if(!r)continue;
  ++n;
 }
 fclose(f);
 free(buf);
 return n;
}


#pragma GCC optimize("O2")


__attribute__((hot))
static inline unsigned rnd(unsigned n){
 return random()%n;
}


#pragma GCC optimize("Os")


__attribute__((cold))
uint32_t hashStr(char*s){
 uint32_t h=0;
 while(*s){
  h*=17;
  h+=tolower(*s);
  ++s;
 }
 return h;
}


__attribute__((cold))
uint32_t*hashWords(char*s){
 uint32_t*m=0;
 unsigned n=0;
 char*p;
 bool b;
 while(*s&&*s!='.'){
  p=s;
  m=real(m,(n+2)*sizeof(uint32_t));
  while(1){
   if(!*s||isspace(*s)||*s=='.'){
    b=*s!=0&&*s!='.';
    *s=0;
    m[n]=hashStr(p);
    ++n;
    if(b){
     while(!*s||isspace(*s))++s;
     break;
    }else goto Done;
   }
   ++s;
  }
 }
 Done:
 m[n]=0;
 return m;
}


__attribute__((cold))
void loadEntry(char*s){
 Entry*e=entry+entries;
 if(!*s)return;
 if(entries>=entryMems)die("Internal counting error");
 e->s=strdup(s);
 e->w=hashWords(s);
 ++entries;
}


__attribute__((cold))
void loadPlaylist(char*name){
 char*buf;
 char*r;
 FILE*f;
 buf=mal(opt.bufSize);
 f=fop(name,"rt");
 while(!feof(f)){
  *buf=0;
  r=fgets(buf,opt.bufSize,f);
  if(!r)continue;
  rtrim(buf);
  if(!*buf)continue;
  loadEntry(buf);
 }
 fclose(f);
 if(opt.verbose)printf("Loaded %u entries\n",entries);
 if(entries<1)die("Empty playlist");
}


__attribute__((cold))
void savePlaylist(char*name){
 Entry*e=entry;
 unsigned n=entries;
 FILE*f=fop(name,"wt");
 while(n--){
  fputs(e->s,f);
  putc('\n',f);
  ++e;
 }
 fclose(f);
}


#pragma GCC optimize("O2")


__attribute__((hot))
int sortWords(const void*va,const void*vb){
 const Word*a=va;
 const Word*b=vb;
 if(a->w > b->w)return 1;
 if(a->w < b->w)return -1;
 return 0;
}


__attribute__((hot))
int findWord(uint32_t w){
 for(int i=0;i<words;++i){
  if(w==word[i].w)return i;
 }
 return -1;
}


/*
__attribute__((hot))
int findWordFast(uint32_t h){
 Word w={
  .w=h,
  .c=0,
 };
 Word*p=bsearch(&w,word,words,sizeof(Word),sortWords);
 return p?p-word:-1;
}
*/


__attribute__((hot))
float getWordScore(uint32_t w){
 int i=findWord(w);///findWordFast using bsearch is slower!
 if(i<0){
  fprintf(stderr,"Word=%u \n",w);
  die("Unknown word");
 }
 return word[i].sc;
}


__attribute__((hot))
float scoreCommon(uint32_t*a,uint32_t*bb){
 uint32_t*b;
 float sc=0;
 while(*a){
  b=bb;
  while(*b){
   if(*a==*b)sc+=getWordScore(*a);
   ++b;
  }
  ++a;
 }
 return sc;
}


__attribute__((hot))
void scoreIt(){
 unsigned i,next;
 score=0;
 if(entries<1)die("Too few entries");
 for(i=0;i<entries-1;++i){
  next=i+1;
  score+=scoreCommon(entry[i].w,entry[next].w);
 }
}


__attribute__((hot))
void mutate(){
 Entry e;
 unsigned a,b,n;
 n=opt.loMut;
 a=opt.hiMut-opt.loMut;
 if(a)n+=rnd(1+a);
 mutations=n;
 while(n--){
  a=rnd(entries);
  b=rnd(entries);
  e=entry[a];
  entry[a]=entry[b];
  entry[b]=e;
 }
}


#pragma GCC optimize("Os")


__attribute__((cold))
void shuffle(){
 unsigned n=opt.shuffle*entries;
 while(n--)mutate();
}


__attribute__((cold))
void initMem(){
 unsigned n=flines(opt.infile);
 if(n<1)die("Empty playlist");
 entryMems=1+n;
 entry=mal(sizeof(Entry)*entryMems);
 if(opt.verbose)printf("Allocated memory for %u lines\n",n);
}


__attribute__((cold))
void addWord(uint32_t w){
 word=real(word,(1+words)*sizeof(Word));
 word[words].w=w;
 word[words].c=1;
 ++words;
}


__attribute__((cold))
void countThisWord(uint32_t w){
 int r=findWord(w);
 if(r>=0)++word[r].c;
 else addWord(w);
}


__attribute__((cold))
void countThese(uint32_t*w){
 while(*w){
  countThisWord(*w);
  ++w;
 }
}


__attribute__((cold))
void removeWords(){
 Word*w=mal(words*sizeof(Word));
 unsigned i,j=0;
 for(i=0;i<words;++i){
  if(word[i].w){
   w[j]=word[i];
   ++j;
  }
 }
 free(word);
 word=real(w,j*sizeof(Word));
 words=j;
}


__attribute__((cold))
void scoreWords(){
 unsigned i;
 for(i=0;i<entries;++i){
  countThese(entry[i].w);
 }
 if(opt.verbose)printf("Found %d words \n",words);
 for(i=0;i<words;++i){
  if(word[i].c>1)word[i].sc=100.0/word[i].c;
  else word[i].w=0;
 }
 removeWords();
 if(opt.verbose)printf("Scored %d words seen multiple times \n",words);
 qsort(word,words,sizeof(Word),sortWords);
}


__attribute__((cold))
bool parseArg(char*a,char*b){
 int n=0;
 if(b)n=atol(b);
 if(*a!='-'||!a[1]||(a[2]&&a[3])){
  BadArg:
  die("Bad arg!");
 }

 switch(a[1]){
  case 'h':
  case '?':
  puts(helpText);
  exit(0);
 }

 if(!b)die("No value given for option");

 switch(a[1]){
  case 'm':
  switch(a[2]){
   case 'l':
   opt.loMut=n;
   break;
   case 'h':
   opt.hiMut=n;
   break;
   default:
   goto UnkOpt;
  }
  break;
 }

 if(a[2])goto BadArg;

 switch(a[1]){
  case 'i':
  opt.infile=b;
  break;
  case 'o':
  opt.outfile=b;
  break;
  case 't':
  opt.timeLimit=n;
  break;
  case 'b':
  opt.bufSize=n;
  break;
  case 'r':
  opt.shuffle=n;
  break;
  case 'v':
  opt.verbose=n;
  break;
  default:
  UnkOpt:
  die("Unknown option");
 }
 return 1;
}


__attribute__((cold))
void parseArgs(int argc,char**argv){
 int r;
 while(--argc>0){
  ++argv;
  r=parseArg(*argv,argc?argv[1]:0);
  argc-=r;
  argv+=r;
 }
}


void warn(char*s){
 printf("Warning: %s \n",s);
}


void warnings(){
 if(opt.bufSize<=0)die("Buffer too small");
 if(opt.bufSize<=32)warn("Buffer is small");
 if(opt.loMut>opt.hiMut)die("Mutation low>high");
}


__attribute__((cold))
int main(int argc,char**argv){
 time_t endTime;
 if(argc<2){
  puts(helpText);
  die("Too few args");
 }
 parseArgs(argc,argv);
 warnings();
 endTime=time(0)+opt.timeLimit;
 srand(endTime);
 initMem();
 loadPlaylist(opt.infile);
 shuffle();
 scoreWords();
 while(1){
  scoreIt();
  if(bestScore<score){
   bestScore=score;
   bestEntry=real(bestEntry,entries*sizeof(Entry));
   memcpy(bestEntry,entry,entries*sizeof(Entry));
   if(opt.verbose){
    printf(
		"Best score %0.1f - Mutations %u   \r",
		bestScore,
		mutations
	);
   }
  }
  memcpy(entry,bestEntry,entries*sizeof(Entry));
  if(endTime<=time(0))break;
  mutate();
 }
 if(opt.verbose)printf("\nFinal score %f \n",bestScore);
 savePlaylist(opt.outfile);
 return 0;
}


