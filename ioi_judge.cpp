/***********************************************************************************
 * IOI Scoring for the SPOJ platform.
 * This program ranks such that:
 *   - There are no penalties for resubmissions
 *   - Contestants are sorted decreasingly by the sum of points for each attempted problem.
       Each problem has a fixed maximum score, which is distributed uniformly across some test sets.
 *
 * To accomodate this scoring scheme within SPOJ:
 * Each contest problem must have a numeric value in the Info field from the contests' screen.
 * Each contest problem must use the #1001 master judge (sum of % solved sets), or a custom one
 * which returns the % of solved sets.
 * WARNING: The #1001 master judge ALWAYS returns AC for a given submission, even if it doesn't pass any tests.
            To avoid confusion, it is recommended to write a custom judge (our use ours). 
 *
 * Authors:
 *   Carlos Joa (Main author)
 *   Roberto Abreu (Minor changes)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

//#include <iostream>

#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <cmath>


using namespace std;

#define EPS 1e-9
#define INCORRECT_SUBMISSION_PENALTY 10

#define LANG_JAVA        10
#define LANG_C           11
#define LANG_RUBY        17
#define LANG_C_SHARP     27
#define LANG_C_PLUSPLUS  41

#define VERDICT_COMPILATION_ERROR  11
#define VERDICT_RUNTIME_ERROR      12
#define VERDICT_TIME_LIMIT_EXCEED  13
#define VERDICT_WRONG_ANSWER       14
#define VERDICT_ACCEPTED           15

#define UNSOLVED         0
#define FULLY_SOLVED     1
#define PARTIALLY_SOLVED 2


int  len;
char buf[2000];

char BASE_PATH[1000];


inline string get_trimmed_line(FILE *fp)
{
   fgets(buf, sizeof(buf), fp);
   for (len=strlen(buf)-1; len >= 0 && isspace(buf[len]); --len);
   buf[len+1] = 0;
   return string(buf);
}

struct Contest
{
   int    starttime, endtime;
   int    sol_limit;
   string code;
   string name;
   Contest(FILE *input, int l)
   {
      fscanf(input, "%d %d %d\n", &starttime, &endtime, &sol_limit);
      l -= 3;

      code = get_trimmed_line(input);
      l--;

      name = get_trimmed_line(input);
      l--;

      while (l-- > 0)
         fgets(buf, 1000, input);
   }
};

struct ProblemParsingException
{
   string msg;
   int id;
   string code, name;
   ProblemParsingException(const char *_msg, int _id = -1,
                           const string& _code = "",
                           const string& _name = "")
      : msg(_msg), id(_id), code(_code), name(_name) {}
};

struct IllegalProblemCategory : ProblemParsingException
{
   IllegalProblemCategory(const char *_msg, int _id = -1,
                          const string& _code = "",
                          const string& _name = "")
      : ProblemParsingException(_msg, _id, _code, _name) {}
};

struct IllFormattedPointValue : ProblemParsingException
{
   IllFormattedPointValue(const char *_msg, int _id = -1,
                          const string& _code = "",
                          const string& _name = "")
      : ProblemParsingException(_msg, _id, _code, _name) {}
};

struct Problem
{
   int    id;
   int    timelimit;
   int    opt;
   string pset;
   int    starttime;
   int    endtime;
   double points;
   string code;
   string name;

   Problem(FILE *input, int l)
      throw(IllegalProblemCategory, IllFormattedPointValue)
   {
      fscanf(input, "%d %d\n", &id, &timelimit);
      l -= 2;

      code = get_trimmed_line(input);
      l--;

      name = get_trimmed_line(input);
      l--;

      fgets(buf, sizeof(buf), input);
      sscanf(buf, "%d", &opt);
      if (opt)
         throw IllegalProblemCategory(
            "Challenge problems are not allowed in this scoring system",
            id, code, name
         );
      l--;

      pset = get_trimmed_line(input);
      l--;

      fgets(buf, sizeof(buf), input);
      sscanf(buf, "%d", &starttime);
      l--;
      
      fgets(buf, sizeof(buf), input);
      sscanf(buf, "%d", &endtime);
      l--;

      fgets(buf, sizeof(buf), input);
      if (sscanf(buf, "%lf", &points) != 1)
         throw IllFormattedPointValue(
            "In the 'Edit contest' menu, please fill in the 'Info' field "
            " for each problem with a floating point numerical value",
            id, code, name
         );
      l--;

      while (l-- > 0)
         fgets(buf, sizeof(buf), input);
   }
};

struct User
{
   int    id;
   string login;
   string name;
   string email_address;
   string institution;
   string info;
   User() {}
   User(FILE *input, int l)
   {
      fscanf(input, "%d\n", &id);
      l--;

      login = get_trimmed_line(input);
      l--;

      name = get_trimmed_line(input);
      l--;

      email_address = get_trimmed_line(input);
      l--;

      institution = get_trimmed_line(input);
      l--;

      info = get_trimmed_line(input);
      l--;

      while (l-- > 0)        
         fgets(buf, sizeof(buf), input);
   }
};

struct UserProblem
{
   int solved;
   double  score;
   int  tries, failed_tries;
   int  time;
   int  date;
   string  strdate;
   UserProblem() 
      : solved(UNSOLVED), tries(0), failed_tries(0), score(0.0), time(0) {}
};

typedef map<int, UserProblem> u_p_map;

struct Result
{
   int     userid;
   int     numsolved;
   int     last_solved;
   double  totalscore;
   int     penalty;
   u_p_map prob; // int - problem id
   Result() : numsolved(0), last_solved(0), totalscore(0.0), penalty(0) {}
   bool operator<(const Result& r) const
   {
      if (fabs( totalscore - r.totalscore ) > EPS)
         return totalscore > r.totalscore;
      if (penalty != r.penalty)
         return penalty < r.penalty;
      if (last_solved != r.last_solved)
         return last_solved < r.last_solved;
      if (numsolved != r.numsolved)
         return numsolved > r.numsolved;
      return userid < r.userid;
   }
};

struct Submission
{
   int    userid;
   int    problemid;
   int    timestamp;
   int    status;
   int    language;
   double score;
   double time;
   string date;
   Submission(FILE *input, int l)
   {
      fscanf(input, "%d %d %d %d %d %lf %lf\n", &userid, &problemid,
                    &timestamp, &status, &language, &score, &time );
      l -= 7;
      if (fabs(100. - score) < EPS)
         score = 100.0;

      date = get_trimmed_line(input);
      l--;

      while (l-- > 0)
         fgets(buf, sizeof(buf), input);
   }
}; 

typedef map<int, Problem> p_map;
typedef map<int, User> u_map;
typedef map<int, Result> ur_map;   // int - user id
typedef map<string, ur_map> pur_map;  // string - problem set
typedef map<int, int> ii_map;
typedef map< string, vector<int> > pset_map;

vector<Problem> problems;
ii_map prob_id2idx;   // maps problem id to index into vector of problems

pset_map problem_sets;

u_map users;
pur_map results;



int main() {      
#ifdef ONLINE_JUDGE
   FILE *input = fdopen(0, "r");
   FILE *output = fdopen(6, "w");
#else
   FILE *input = stdin;
   FILE *output = stdout;
#endif


// contest data
   int cont_n = 0;
   fscanf(input, "%d\n", &cont_n);
   Contest contest(input, cont_n);

   strcpy(BASE_PATH, contest.code.c_str());


// problems data
   int pr_n, pr_l;
   fscanf(input, "%d %d\n", &pr_n, &pr_l);
   for (int i = 0; i < pr_n; i++)
   {
      try
      {
         Problem pr(input, pr_l);
         problems.push_back(pr);
         prob_id2idx[pr.id] = i;
         problem_sets[pr.pset].push_back(i);
      }
      catch (ProblemParsingException& ex)
      {
         fprintf(output, "HTML<h3>Contest judge error.</h3>"
                         "<p>%s.\n"
                         "<p>Problem Id: %d (%d of %d)\n"
                         "<p>Name : %s - %s\n",
                         ex.msg.c_str(), ex.id, i+1, pr_n,
                         ex.code.c_str(),
                         ex.name.c_str());

         return 1;
      }
   }

// users data
   int u_n, u_l, u_id;
   fscanf(input, "%d %d\n", &u_n, &u_l);
   for (int i = 0; i < u_n; i++)
   {
      User usr(input, u_l);
      users.insert(make_pair(usr.id, usr));
   }

// submissions
   int subm_s, subm_l;
   fscanf(input, "%d %d\n", &subm_s, &subm_l);

   fprintf(output, "%u\n",
           (unsigned int) (subm_s * problem_sets.size())); // number of rankings

   for (int j = 0; j < subm_s; j++)
   {
      results.clear();

      fgets(buf, sizeof(buf), input);

      int subm_n;
      fscanf(input, "%d\n", &subm_n);
      for (int i = 0; i < subm_n; i++)
      {
         Submission s(input, subm_l);
         if (users.find(s.userid) == users.end()) continue;
         const Problem& prob = problems[ prob_id2idx[s.problemid] ];
         Result& res = results[prob.pset][s.userid];
         res.userid = s.userid;
         UserProblem& uprob = res.prob[s.problemid];
         uprob.tries++;
         if (uprob.solved != FULLY_SOLVED)
         {
             if (fabs(100. - s.score) < EPS) {
                 res.numsolved++;
                 uprob.solved  = FULLY_SOLVED;
             }
             else {
                 uprob.solved  = PARTIALLY_SOLVED;
                 uprob.failed_tries++;
             }
             uprob.score = s.score;
             uprob.date = s.timestamp;
             uprob.time    = s.timestamp - prob.starttime;
         }
      }

      for (pset_map::const_iterator ps_it = problem_sets.begin();
           ps_it != problem_sets.end(); ++ps_it)
      {
         vector<Result> vres;
         const vector<int>& vprob = ps_it->second;
         ur_map& mres = results[ps_it->first];
         for (ur_map::iterator it = mres.begin(); it != mres.end(); ++it)
         {
            Result& res = it->second;
            for (int j = 0; j < vprob.size(); ++j)
            {
               const Problem& prob = problems[ vprob[j] ];
               u_p_map::const_iterator up = res.prob.find(prob.id);
               if (up != res.prob.end())
               {
                  if (up->second.solved != UNSOLVED)
                  {
                     res.totalscore += prob.points * (up->second.score/100.);
                     res.penalty    += up->second.failed_tries
                                       * INCORRECT_SUBMISSION_PENALTY * 60;
                     res.last_solved = max(res.last_solved, up->second.time);
                  }
               }
            }
            res.penalty += res.last_solved;
            vres.push_back(res);
         }

         sort(vres.begin(), vres.end());

         int fields = 3 + vprob.size();
         fprintf(output, "%d\n", fields); // number of fields in ranking      
         fprintf(output, "Posici&oacute;n\n"
                         "&nbsp;&nbsp;&nbsp;Competidor&nbsp;&nbsp;&nbsp;\n");
         for (int j = 0; j < vprob.size(); ++j)
         {
            fprintf(output, "<a href='/%s/problems/%s'>%s</a><br>%.2f\n",
                    BASE_PATH, problems[vprob[j]].code.c_str(),
                    problems[vprob[j]].code.c_str(),
                    problems[vprob[j]].points);
         }
         fprintf(output, "&nbsp;Puntuaci&oacute;n&nbsp;\n");

         fprintf(output, "%d\n", (int) vres.size());

         double prev_score = 0.0;
         int prev_penalty = 0;
         int rank = 1;
         for (int i = 0; i < vres.size(); ++i)
         {
            Result& res = vres[i];
            if (res.totalscore < prev_score-EPS)
                rank = i + 1;

            const User& usr = users[res.userid];
            fprintf(output, "%d\n<a href='/%s/users/%s'>%s</a><br>%s\n",
                    rank, BASE_PATH, usr.login.c_str(),
                    usr.login.c_str(),
                    usr.institution.c_str());
            for (int j = 0; j < vprob.size(); ++j)
            {
               const Problem& prob = problems[ vprob[j] ];
               u_p_map::const_iterator up = res.prob.find(prob.id);
               if (up != res.prob.end())
               {
                  if (up->second.solved != UNSOLVED)
                  {
                     fprintf(output, "%.2f<br><a href='/%s/status/%s,%s/'>ver</a>\n",
                             prob.points * (up->second.score/100.),
                             BASE_PATH, prob.code.c_str(),
                             usr.login.c_str());
                  }
                  else
                  {
                     fprintf(output, "%.2f<br><a href='/%s/status/%s,%s/'>ver</a>\n",
                             prob.points * (up->second.score/100.),
                             BASE_PATH, prob.code.c_str(),
                             usr.login.c_str(),
                             up->second.failed_tries);
                  }
               }
               else
                  fputs("-\n", output);
            }
            fprintf(output, "%.2f\n", res.totalscore);
            prev_score   = res.totalscore;
            prev_penalty = res.penalty;
         }
      }
   }
   fclose(output);
      
   return 0;
}
/*
 * input format:
 *  - contest info
 *  - problems info
 *  - users info
 *  - submissions
 *
 * Contest info:
 * integer N - number of lines.
 * N lines, each with one value:
 * start // int - timestamp
 * end      // int - timestamp
 * solution limit
 * code     // string
 * name     // string
 * ?        // date
 * ...
 *
 * Problems info:
 * integer N - number of problems
 * integer L - number of lines for each problem
 * N*L lines, each with one value:
 * id    // int
 * timelimit   // int
 * code     // string
 * name     // string
 * challenge_flag  //  0 = classical   1 = challenge
 * problem_set    // problem set name
 * star_time   // int - timestamp
 * end_time    // int - timestamp
 * points      // double
 * author_id   // int
 * resource    // string
 * ...
 *
 * Users info:
 * integer N - number of users
 * integer L - number of lines for each user
 * N*L lines, each with one value:
 * id    // int
 * login // string
 * name     // string
 * email address   // string
 * institution   // string
 * info          // string
 * ???
 * ???         // date
 * ???         // date
 * 
 * Submissions:
 * integer S - number of submission series
 * integer L - number of lines for each submission
 * For each serie:
 * info     // string
 * integer N - number of submissions in current serie.
 * N*L lines, each with one value:
 * user id  // int
 * problem id  // int
 * date     // int (timestamp)
 * status   // int
 * language // int
 * score    // floating point
 * time     // floating point
 * string date    // string
 * submission_id   // int
 * ???          // int
 * extra status info   // string
 * memory        // int
 * ...
 * 
 */