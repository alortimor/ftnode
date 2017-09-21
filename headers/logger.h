#ifndef LOGGER_H
#define LOGGER_H
#include <memory>
#include <mutex>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

inline std::string time_stamp() {
	std::stringstream buffer;
	using std::chrono::system_clock;
	std::time_t tt = system_clock::to_time_t(system_clock::now());

	struct std::tm * ptm = std::localtime(&tt);
	buffer << std::put_time(ptm, "%Y%m%d%H%M%S");
	return buffer.str();
}

class threadsafe_log {
  private:
    std::string file_path;
    std::mutex mx;
    const std::chrono::time_point<std::chrono::system_clock>  start {std::chrono::system_clock::now()}; // since the loggers inception

  public:
    explicit threadsafe_log (const std::string & path, const std::string & file_name) {
		if(!file_name.empty())
			file_path = path + "/";
	  	file_path += file_name + "_" + time_stamp() + ".log";
      std::ofstream file{file_path, std::fstream::app}; // open file in append mode so nothing is ever overwritten
      if(file) {
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        file << "Start: " << d.count() << std::endl; // write initial duration so as to simplify benchmarking
      }
    }

    threadsafe_log (const threadsafe_log &) = delete;
    threadsafe_log & operator=(const threadsafe_log &) = delete;

    ~threadsafe_log() {
      if(!file_path.empty()) {
        std::ofstream file{file_path, std::fstream::app};
        if(file) {
          auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
          file << "End: " << d.count() << std::endl;
        }
      }
    }
    // return value: 0 in success, 2 in failure
    int write (const std::string & msg) {
      // ensure the blocking duration is not included in the calculation written to file.
      auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start); // calculate duration prior to blocking
      // now block if other threads are writing as well
      {
        std::lock_guard<std::mutex> lk(mx);
        if(!file_path.empty()) {
          std::ofstream file{file_path, std::fstream::app};
          if(file)
            file << d.count() << ": " << msg << std::endl; // assume a new line each time
          else          
            return 2;
        }        
      }
      return 0;
    }
};

class logger {
  private:
    std::unique_ptr<threadsafe_log> lg;
  public:
    logger (std::unique_ptr<threadsafe_log>&& logger) : lg(std::move(logger)) {  }
    // return value: 0 in success, 2 in failure
    int write (const std::string & msg ) { return lg->write(msg); }
};

///////////////////////////////////////////////////////////////////////////////////
// Here define which loggers you want to use. Comment out the loggers
// you do not want to use/compile. If for example USE_LOG1 is commented out 
// then log() is not compiled at all in the code.
// USE_EXCEPTION_LOG: generally use loggers. If no loggers used, comment this out.
// Note: This needs to be on if at least one logger is used.
// 
// USE_LOG1: use the first logger (logger1)
// An example 1: Using two first loggers:
// #define USE_EXCEPTION_LOG
// #define USE_LOG1
// #define USE_LOG2 
// //#define USE_LOG3 
// //#define USE_LOG4 
//
// An example 2: Using no loggers (thus no log-related code is being compiled)
// //#define USE_EXCEPTION_LOG
// #define USE_LOG1 
// #define USE_LOG2 
// #define USE_LOG3 
// #define USE_LOG4 

#define USE_EXCEPTION_LOG
#define USE_LOG1 
#define USE_LOG2 
#define USE_LOG3 
#define USE_LOG4 
///////////////////////////////////////////////////////////////////////////////////


#ifdef USE_EXCEPTION_LOG

class log_file_path {
private:
  friend int log_1_(const std::string & msg);
  friend int log_2_(const std::string & msg);
  friend int log_3_(const std::string & msg);
  friend int log_4_(const std::string & msg);
  
  friend void set_log1_file_path_(const std::string & path_, const std::string & file_name_);
  friend void set_log2_file_path_(const std::string & path_, const std::string & file_name_);
  friend void set_log3_file_path_(const std::string & path_, const std::string & file_name_);
  friend void set_log4_file_path_(const std::string & path_, const std::string & file_name_);
  
  std::string path;
  std::string file_name;
  
public:
  log_file_path(const std::string & path_="", const std::string & file_name_="") :
    path{path_}, file_name{file_name_} {}
};

extern log_file_path log1_file_path;
extern log_file_path log2_file_path;
extern log_file_path log3_file_path;
extern log_file_path log4_file_path;

inline void set_log1_file_path_(const std::string & path_, const std::string & file_name_) {
  log1_file_path.path = path_;
  log1_file_path.file_name = file_name_;
}

inline void set_log2_file_path_(const std::string & path_, const std::string & file_name_) {
  log2_file_path.path = path_;
  log2_file_path.file_name = file_name_;
}

inline void set_log3_file_path_(const std::string & path_, const std::string & file_name_) {
  log3_file_path.path = path_;
  log3_file_path.file_name = file_name_;
}

inline void set_log4_file_path_(const std::string & path_, const std::string & file_name_) {
  log4_file_path.path = path_;
  log4_file_path.file_name = file_name_;
}

// return value: 0 in success, 2 in failure
inline int log_1_(const std::string & msg)
{
  static std::unique_ptr<logger> exception_log{std::make_unique<logger>(
    std::make_unique<threadsafe_log>(log1_file_path.path, log1_file_path.file_name)) };
  static std::string cur_path;
  static std::string cur_file_name;
  // if file path is changed create a new exception_log
  /*if(cur_path != log1_file_path.path || cur_file_name != log1_file_path.file_name) {
    cur_path = log1_file_path.path;
    cur_file_name = log1_file_path.file_name;
    exception_log = std::make_unique<logger>(
      std::make_unique<threadsafe_log>(log1_file_path.path, log1_file_path.file_name));
  }*/
    
	return exception_log->write(msg);
}

// return value: 0 in success, 2 in failure
inline int log_2_(const std::string & msg)
{
  static std::unique_ptr<logger> exception_log{ std::make_unique<logger>(
    std::make_unique<threadsafe_log>(log2_file_path.path, log2_file_path.file_name))};
  static std::string cur_path;
  static std::string cur_file_name;
  // if file path is changed create a new exception_log
 /* if(cur_path != log2_file_path.path || cur_file_name != log2_file_path.file_name) {
    cur_path = log2_file_path.path;
    cur_file_name = log2_file_path.file_name;    
    exception_log = std::make_unique<logger>(
      std::make_unique<threadsafe_log>(log2_file_path.path, log2_file_path.file_name));
  }*/
  
	return exception_log->write(msg);
}

// return value: 0 in success, 2 in failure
inline int log_3_(const std::string & msg)
{
  static std::unique_ptr<logger> exception_log{ std::make_unique<logger>(
    std::make_unique<threadsafe_log>(log3_file_path.path, log3_file_path.file_name))};
  static std::string cur_path;
  static std::string cur_file_name;
  // if file path is changed create a new exception_log
  /*if(cur_path != log3_file_path.path || cur_file_name != log3_file_path.file_name) {
    cur_path = log3_file_path.path;
    cur_file_name = log3_file_path.file_name;    
    exception_log = std::make_unique<logger>(
      std::make_unique<threadsafe_log>(log3_file_path.path, log3_file_path.file_name));
  }*/
    
	return exception_log->write(msg);
}

// return value: 0 in success, 2 in failure
inline int log_4_(const std::string & msg)
{
  static std::unique_ptr<logger> exception_log{ std::make_unique<logger>(
    std::make_unique<threadsafe_log>(log4_file_path.path, log4_file_path.file_name))};
  static std::string cur_path;
  static std::string cur_file_name;
  // if file path is changed create a new exception_log
  /*if(cur_path != log4_file_path.path || cur_file_name != log4_file_path.file_name) {
    cur_path = log4_file_path.path;
    cur_file_name = log4_file_path.file_name;    
    exception_log = std::make_unique<logger>(
      std::make_unique<threadsafe_log>(log4_file_path.path, log4_file_path.file_name));
  }*/
    
	return exception_log->write(msg);
}

// in all log- functions: return value: 0 in success, 2 in failure
#ifdef USE_LOG1
  #define log_1(x) log_1_(x)
#else
  #define log_1(x)
#endif

#ifdef USE_LOG2
  #define log_2(x) log_2_(x)
#else
  #define log_2(x)
#endif

#ifdef USE_LOG3
  #define log_3(x) log_3_(x)
#else
  #define log_3(x)
#endif

#ifdef USE_LOG4
  #define log_4(x) log_4_(x)
#else
  #define log_4(x)
#endif
  
// set the full file path of log_ loggers. Do this before using them. 
// If file paths not set they use default filepaths (see cpp file: log1_file_path ...)
#define set_log1_file_path(x, y)  set_log1_file_path_(x, y)
#define set_log2_file_path(x, y)  set_log2_file_path_(x, y)
#define set_log3_file_path(x, y)  set_log3_file_path_(x, y)
#define set_log4_file_path(x, y)  set_log4_file_path_(x, y)

#else

#define log_1(x)
#define log_2(x)
#define log_3(x)
#define log_4(x)

#define set_log1_file_path(x, y)  
#define set_log2_file_path(x, y) 
#define set_log3_file_path(x, y)  
#define set_log4_file_path(x, y)  

#endif



#endif // LOGGER_H



/*#ifndef LOGGER_H
#define LOGGER_H
#include <memory>
#include <mutex>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <iostream>

// Used to log exceptions
inline std::string time_stamp() {
	std::stringstream buffer;
	using std::chrono::system_clock;
	std::time_t tt = system_clock::to_time_t(system_clock::now());

	struct std::tm * ptm = std::localtime(&tt);
	buffer << std::put_time(ptm, "%Y%m%d%H%M%S");
	return buffer.str();
}

class threadsafe_log {
  private:
    std::string file_path;
    std::mutex mx;
    const std::chrono::time_point<std::chrono::system_clock>  start {std::chrono::system_clock::now()}; // since the loggers inception

  public:
    explicit threadsafe_log (const std::string & path, const std::string & file_name) {
		if(!file_name.empty())
			file_path = path + "/";
	  	file_path += file_name + "_" + time_stamp() + ".log";
      std::ofstream file{file_path, std::fstream::app}; // open file in append mode so nothing is ever overwritten
      if(file) {
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        file << "Start: " << d.count() << std::endl; // write initial duration so as to simplify benchmarking
      }
      else 
        std::cerr << "Failed to create log file: " << file_path << std::endl;
    }

    threadsafe_log (const threadsafe_log &) = delete;
    threadsafe_log & operator=(const threadsafe_log &) = delete;

    ~threadsafe_log() {
      if(!file_path.empty()) {
        std::ofstream file{file_path, std::fstream::app};
        if(file) {
          auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
          file << "End: " << d.count() << std::endl;
        }
        else 
          std::cerr << "Failed to open/create log file: " << file_path << std::endl;
      }
    }

    void write (const std::string & msg) {
      // ensure the blocking duration is not included in the calculation written to file.
      auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start); // calculate duration prior to blocking
      // now block if other threads are writing as well
      {
        std::lock_guard<std::mutex> lk(mx);
        if(!file_path.empty()) {
          std::ofstream file{file_path, std::fstream::app};
          if(file)
            file << d.count() << ": " << msg << std::endl; // assume a new line each time
          else          
            std::cerr << "Failed to open/create log file: " << file_path << std::endl;
        }        
      }
    }
};

class logger {
  private:
    std::shared_ptr<threadsafe_log> lg;
  public:
    logger (std::shared_ptr<threadsafe_log> logger) : lg(logger) {  }
    void write (const std::string & msg ) { lg->write(msg); }
};

#define EXCEPTION_LOG

#ifdef EXCEPTION_LOG
inline void log_(const std::string & msg)
{
	static logger exception_log( std::make_shared<threadsafe_log>("log", "exceptions"));
	exception_log.write(msg);
}

#define log(x) log_(x)
#else
#define log(x)
#endif

#endif // LOGGER_H
*/
