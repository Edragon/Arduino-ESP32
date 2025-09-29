#include <TokenIterator.h>

#include <memory>

#ifndef _URL_TOKEN_BINDINGS_H
#define _URL_TOKEN_BINDINGS_H

class UrlTokenBindings {
public:
  UrlTokenBindings(
      std::shared_ptr<TokenIterator> patternTokens,
      std::shared_ptr<TokenIterator> requestTokens
  );
  UrlTokenBindings(
      std::shared_ptr<TokenIterator> patternTokens,
      const char* requestPath
  );

  bool hasBinding(const char* key) const;
  const char* get(const char* key) const;

private:
  std::shared_ptr<TokenIterator> patternTokens;
  std::shared_ptr<TokenIterator> requestTokens;
};

#endif
