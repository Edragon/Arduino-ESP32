#include <string.h>
#include <UrlTokenBindings.h>

#include <memory>
#include <utility>

UrlTokenBindings::UrlTokenBindings(
    std::shared_ptr<TokenIterator> patternTokens,
    std::shared_ptr<TokenIterator> requestTokens
) : patternTokens(std::move(patternTokens)),
    requestTokens(std::move(requestTokens))
{ }

UrlTokenBindings::UrlTokenBindings(
    std::shared_ptr<TokenIterator> patternTokens,
    const char *requestPath
): patternTokens(std::move(patternTokens)),
   requestTokens(std::make_shared<TokenIterator>(requestPath, strlen(requestPath), '/'))
{ }

bool UrlTokenBindings::hasBinding(const char* searchToken) const {
  patternTokens->reset();

  while (patternTokens->hasNext()) {
    const char* token = patternTokens->nextToken();

    if (token[0] == ':' && strcmp(token+1, searchToken) == 0) {
      return true;
    }
  }

  return false;
}

const char* UrlTokenBindings::get(const char* searchToken) const {
  patternTokens->reset();
  requestTokens->reset();

  while (patternTokens->hasNext() && requestTokens->hasNext()) {
    const char* token = patternTokens->nextToken();
    const char* binding = requestTokens->nextToken();

    if (token[0] == ':' && strcmp(token+1, searchToken) == 0) {
      return binding;
    }
  }

  return nullptr;
}
