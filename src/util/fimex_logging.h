#ifndef FIMEX_LOGGING_H
#define FIMEX_LOGGING_H

void init_fimex_logging();
void finish_fimex_logging();

class FimexLoggingAdapter {
public:
  FimexLoggingAdapter()
    { init_fimex_logging(); }
  ~FimexLoggingAdapter()
    { finish_fimex_logging(); }

private:
  FimexLoggingAdapter(const FimexLoggingAdapter&) = delete;
  FimexLoggingAdapter& operator=(const FimexLoggingAdapter&) = delete;
};

#endif // FIMEX_LOGGING_H
