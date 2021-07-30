#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include "../../header/log/log.h"
#include "../../header/http/http.h"

namespace Http {
  size_t addrlen(const char *start, const char *end) {
    return (size_t)(end - start);
  }

  std::string extention_name(const std::string &basename) {
    std::string extention("");
    size_t pos = basename.find_last_of('.');
    if (pos == std::string::npos) 
      return extention;
    return basename.substr(pos + 1);
  }

  const char *find_content(const char *start, const char *end, char endc, size_t &contlen, size_t &sumlen) {
    size_t contentlen = 0;
    const char *contstart = NULL;
    for (const char *mstart = start; mstart < end; mstart++) {
      if (contstart == NULL) {
	if (*mstart != ' ') {
	  contstart = mstart;
	  contentlen = 1;
	}
      } else {
	if (*mstart == endc) {
	  contlen = contentlen;
	  sumlen = addrlen(start, mstart);
	  return contstart;
	}
	contentlen++;
      }
    }
    return NULL;
  }

  const char *find_line(const char *start, const char *end) {
    for (auto lstart = start; lstart < (end - 1); lstart++) {
      if (lstart[0] == '\r' && lstart[1] == '\n') {
	return &lstart[2];
      }
    }
    return NULL;
  }

  const char *find_headline(const char *start, const char *end) {
    for (auto hstart = start; hstart < (end - 3); hstart++) {
      if (hstart[0] == '\r' && hstart[1] == '\n' && hstart[2] == '\r' && hstart[3] == '\n') {
	return &hstart[4];
      }
    }
    return NULL;
  } 

  const char *http_content_type(const std::string &extention) {
    if (extention.compare("html") == 0x00) 
      return HTTP_HEAD_HTML_TYPE;
    else if (extention.compare("css") == 0x00) 
      return HTTP_HEAD_CSS_TYPE;
    else if (extention.compare("gif") == 0x00)
      return HTTP_HEAD_GIF_TYPE;
    else if (extention.compare("jpg") == 0x00)
      return HTTP_HEAD_JPG_TYPE;
    else if (extention.compare("png") == 0x00)
      return HTTP_HEAD_PNG_TYPE;
    return NULL;
  }

  CHttpRequest::CHttpRequest(): m_strerr("success"), m_body(NULL), m_bodylen(0){}
  CHttpRequest::~CHttpRequest() {
    if (m_body) {
      delete m_body;
    }
  }

  int CHttpRequest::load_packet(const char *msg, size_t msglen) {
    const char *remainMsg = msg;
    const char *endMsg = msg + msglen;
    const char *endline = find_line(remainMsg, endMsg);

    if (endline == NULL) {
      set_strerror("startline not found");
      return -1;
    }

    if (parse_startline(remainMsg, endMsg) < 0) {
      set_strerror("invalid startline");
      return -1;
    }

    remainMsg = endline;
    const char *headline_end = find_headline(remainMsg, endMsg);
    if (headline_end == NULL) {
      set_strerror("headers not found");
      return -1;
    }

    if (parse_headers(remainMsg, endMsg) < 0) {
      set_strerror("parse headers error");
      return -1;
    }

    remainMsg = headline_end;
    if (parse_body(remainMsg, endMsg) < 0) {
      set_strerror("parse body error");
    }
    
    return 0;
  }

  const std::string &CHttpRequest::start_line() {
    return m_startline;
  }

  const std::string &CHttpRequest::method() {
    return m_method;
  }

  const std::string &CHttpRequest::url() {
    return m_url;
  }

  const std::string &CHttpRequest::version() {
    return m_version;
  }

  const HttpHead_t &CHttpRequest::headers() {
    return m_headers;
  }

  bool CHttpRequest::has_head(const std::string &name) {
    auto it = m_headers.find(name);
    if (it == m_headers.end())
      return false;
    return true;
  }

  const std::string &CHttpRequest::head_content(const std::string &name) {
    const static std::string nullstring("");
    auto it = m_headers.find(name);
    if (it == m_headers.end())
      return nullstring;
    return it->second;
  }

  const size_t CHttpRequest::body_len() {
    return m_bodylen;
  }

  const char *CHttpRequest::body() {
    return m_body;
  }

  const char *CHttpRequest::strerror() {
    return m_strerr.c_str();
  }

  void CHttpRequest::set_strerror(const char *str) {
    m_strerr = str;
  }

  int CHttpRequest::parse_startline(const char *start, const char *end) {
    size_t contlen = 0, sumlen = 0;
    const char *cont = NULL, *remainBuff = start;

    cont = find_content(remainBuff, end, '\r', contlen, sumlen);
    if (cont == NULL)
      return -1;
    m_startline = std::string(cont, contlen);

    cont = find_content(remainBuff, end, ' ', contlen, sumlen);
    if (cont == NULL) 
      return -1;
    
    m_method = std::string(cont, contlen);
    remainBuff += sumlen;

    cont = find_content(remainBuff, end, ' ', contlen, sumlen);
    if (cont == NULL)
      return -1;
    m_url = std::string(cont, contlen);
    remainBuff += sumlen;

    cont = find_content(remainBuff, end, '\r', contlen, sumlen);
    if (cont == NULL)
      return -1;
    m_version = std::string(cont, contlen);

    return 0;
  }

  int CHttpRequest::parse_headers(const char *start, const char *end) {
    size_t contlen = 0, sumlen = 0;
    const char *line_start = start;
    std::string head, attr;
    m_headers.clear();
    
    while (true) {
      const char *line_end = find_line(line_start, end);
      if (line_end == NULL)
	return -1;
      else if (line_end == end) break;
      
      const char *headstart = find_content(line_start, line_end, ':', contlen, sumlen);
      if (headstart == NULL)
	return -1;
      head = std::string(headstart, contlen);

      const char *attstart = line_start + sumlen + 0x01;
      attstart = find_content(attstart, line_end, '\r',contlen, sumlen);
      if (attstart == NULL)
	return -1;
      attr = std::string(attstart, contlen);

      line_start = line_end;
      m_headers[head] = attr;
    }

    return 0;
  }

  int CHttpRequest::parse_body(const char *start, const char *end) {
    size_t bodylen = addrlen(start, end);
    if (bodylen == 0x00) 
      return 0;

    char *buff = new char [bodylen];
    if (buff == NULL)
      return -1;
    memcpy(buff, start, bodylen);

    if (m_body != NULL) 
      delete m_body;

    m_body = buff;
    m_bodylen = bodylen;
    return 0;
  }

  CHttpRespose::CHttpRespose() {
    m_package.body = NULL;
    m_package.bodylen = 0x00;
    m_package.data = NULL;
    m_package.datalen = 0x00;
    m_package.dirty = true;
  }

  CHttpRespose::~CHttpRespose() {
    if (m_package.body != NULL)
      delete [] m_package.body;
    if (m_package.data != NULL)
      delete [] m_package.data;
  }

  int CHttpRespose::set_version(const std::string &version) {
    m_package.version = version;
    m_package.dirty = true;
    return 0;
  }

  int CHttpRespose::set_status(const std::string &status, const std::string &reason) {
    m_package.status = status;
    m_package.reason = reason;
    m_package.dirty = true;
    return 0;
  }

  int CHttpRespose::add_head(const std::string &name, const std::string &attr) {
    if (name.empty() || attr.empty())
      return -1;

    m_package.headers[name] = attr;
    m_package.dirty = true;
    return 0;
  }

  int CHttpRespose::del_head(const std::string &name) {
    auto it = m_package.headers.find(name);
    if (it == m_package.headers.end()) 
      return -1;

    m_package.headers.erase(it);
    m_package.dirty = true;
    return 0;
  }

  int CHttpRespose::set_body(const char *body, size_t bodylen) {
    if (body == NULL || bodylen == 0x00 || bodylen > BODY_MAXSIZE) 
      return -1;

    char *buff = new char [bodylen];
    assert(buff != NULL);

    memcpy(buff, body, bodylen);
    if (m_package.body != NULL) 
      delete [] m_package.body;

    m_package.body = buff;
    m_package.bodylen = bodylen;
    m_package.dirty = true;
    return 0;
  }

  size_t CHttpRespose::size() {
    if (m_package.dirty) {
      m_package.totalsize = startline_stringsize() + headers_stringsize();
      m_package.totalsize += m_package.bodylen;
    }
    return m_package.totalsize;
  }

  const char *CHttpRespose::serialize() {
    if (!m_package.dirty) 
      return m_package.data;

    size_t totalsize = size();
    char *buffReserver = new char[totalsize];
    assert(buffReserver != NULL);
    
    char *buff = buffReserver;
    int nprint = snprintf(buff, totalsize, "%s %s %s\r\n", m_package.version.c_str(), m_package.status.c_str(), m_package.reason.c_str());
    if (nprint < 0)
      goto ErrorHandle;

    totalsize -= nprint;
    buff += nprint;

    for (auto it:m_package.headers) {
      const std::string &name = it.first;
      const std::string &attr = it.second;

      nprint = snprintf(buff, totalsize, "%s: %s\r\n", name.c_str(), attr.c_str());
      if (nprint < 0)
	goto ErrorHandle;

      totalsize -= nprint;
      buff += nprint;
    }
    nprint = snprintf(buff, totalsize, "\r\n");
    if (nprint < 0) 
      goto ErrorHandle;

    totalsize -= nprint;
    buff += nprint;

    memcpy(buff, m_package.body, totalsize);
    if (totalsize != m_package.bodylen) {
      error("http.cpp 350: body copy error, target %ld, actually %ld\n", m_package.bodylen, totalsize);
    }
    if (m_package.data != NULL)
      delete m_package.data;
    m_package.data = buffReserver;
    m_package.dirty = false;
    return m_package.data;

ErrorHandle:
    delete [] buffReserver;
    return NULL;
  }

  size_t CHttpRespose::startline_stringsize() {
    const size_t otherchar_size = 4;
    size_t total_size = otherchar_size + m_package.version.size();
    total_size += m_package.status.size() + m_package.reason.size();
    return total_size;
  }

  size_t CHttpRespose::headers_stringsize() {
    const size_t otherchar_size = 4;
    const size_t head_terminatorsize = 2;
    
    size_t stringSize = 0;
    for (auto it:m_package.headers) {
      const std::string &name = it.first;
      const std::string &attr = it.second;

      stringSize += name.size() + attr.size() + otherchar_size;
    }

    stringSize += head_terminatorsize;
    return stringSize;
  }
}
