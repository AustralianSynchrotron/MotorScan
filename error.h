#ifndef _ERROR_H
#define _ERROR_H

#include <QtCore>

#ifdef QT_QTGUI_MODULE_H
#include <QMessageBox>
#endif

#include <iostream>

/// \defgroup Error Error handling.
/// Functions in this group are used for the
/// error parsing, printing, throwing and so on.
///
/// @{


/// Error type.
class Err {
public:

  /// Error severity
  typedef enum {
    INFO,                        ///< Warning
    WARN,                        ///< Error
    ERR                          ///< Fatal error
  } ErrTp ;

private:
  ErrTp terr;                  ///< Error severity
  QString modname;
  QString message;             ///< The message which describes the error

public:

  inline Err(ErrTp _terr, const QString & msg, const QObject * _parent=0)
    : terr(_terr), message(msg) {
    if ( ! _parent || _parent->objectName().isEmpty() )
      modname = "no-name";
    else
      modname = _parent->objectName();
  }

  inline Err(ErrTp _terr, const QString & msg, const QString & _modname)
    : terr(_terr), modname(_modname), message(msg) { }


  /// Return error severity
  inline ErrTp type() const {
    return terr;
  }

  inline QString form() const {
    QString repstr;
    switch ( terr ) {
    case INFO:  repstr += "FYI.";     break;
    case WARN:   repstr += "WARNING!";       break;
    case ERR: repstr += "ERROR!"; break;
    }
    return repstr += " In module \'" + modname + "\'. " + message;
  }

  inline void report() {
    std::cerr << form().toStdString() << std::endl << std::flush;
  }


#ifdef QT_QTGUI_MODULE_H
  inline void popup() {
    switch (terr) {
    case INFO:
      QMessageBox::information ( 0 , modname, form(),
                                 QMessageBox::Ok ) ;
      break;
    case WARN:
      QMessageBox::warning ( 0, modname, form(),
                             QMessageBox::Ok) ;
      break;
    case ERR:
      QMessageBox::critical ( 0, modname, form(),
                              QMessageBox::Abort) ;
      break;
    }
  }
#endif

};



static inline void
throw_error(const QString & msg, const QObject * parent=0) {
  Err err(Err::ERR, msg, parent);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#endif
  throw err;
}

static inline Err
warn(const QString & msg, const QObject * parent=0) {
  Err err(Err::WARN, msg, parent);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#else
#endif
  return err;
}

static inline Err
inform(const QString & msg, const QObject * parent=0) {
  Err err(Err::INFO, msg, parent);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#endif
  return err;
}

static inline void
throw_error(const QString & msg, const QString & modname) {
  Err err(Err::ERR, msg, modname);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#endif
}

static inline Err
warn(const QString & msg, const QString & modname) {
  Err err(Err::WARN, msg, modname);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#endif
  return err;
}

static inline Err
inform(const QString & msg, const QString & modname) {
  Err err(Err::INFO, msg, modname);
  err.report();
#ifdef QT_QTGUI_MODULE_H
  err.popup();
#endif
  return err;
}

/// @}


#endif
