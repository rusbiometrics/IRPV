#ifndef IRPVHELPER_H
#define IRPVHELPER_H

#include <cmath>
#include <cstring>
#include <iostream>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QImage>
#include <QDir>

#include "irpv.h"

//---------------------------------------------------

struct BiometricTemplate
{
    BiometricTemplate() {} // default constructor is needed for std::vector<BiometricTemplate>

    BiometricTemplate(size_t _label, IRPV::TemplateRole _role, std::vector<uint8_t> &&_data) :
        label(_label),
        role(_role),
        data(std::move(_data)) {}

    BiometricTemplate(BiometricTemplate&& other) :
        label(other.label),
        role(other.role),
        data(std::move(other.data)) {}

    BiometricTemplate& operator=(BiometricTemplate&& other)
    {
        if (this != &other) {
            label = other.label;
            role = other.role;
            data = std::move(other.data);
        }
        return *this;
    }

    size_t               label;
    IRPV::TemplateRole   role;
    std::vector<uint8_t> data;
};

//---------------------------------------------------

inline std::ostream&
operator<<(
    std::ostream &s,
    const QImage::Format &_format)
{
    switch (_format) {
        case QImage::Format_ARGB32:
            return (s << "Format_ARGB32");
        case QImage::Format_RGB32:
            return (s << "Format_RGB32");
        case QImage::Format_RGB888:
            return (s << "Format_RGB888");
        case QImage::Format_Grayscale8:
            return (s << "Format_Grayscale8");
        default:
            return (s << "Undefined");
    }
}

//---------------------------------------------------

inline std::ostream&
operator<<(
    std::ostream &s,
    const QString &_qstring)
{
    return s << _qstring.toLocal8Bit().constData();
}

//---------------------------------------------------

IRPV::Image readimage(const QString &_filename, QImage::Format _mTARgetformat=QImage::Format_RGB888, bool _verbose=false)
{
    if(_verbose)
        std::cout << _filename << std::endl;

    QImage _qimg;
    if(!_qimg.load(_filename)) {
        if(_verbose)
            std::cout << "Can not load or decode!!! Empty image will be returned" << std::endl;
        return IRPV::Image();
    }

    QImage _tmpqimg;
    if(_qimg.format() == _mTARgetformat) {
        _tmpqimg = _qimg;
    } else {
        _tmpqimg = std::move(_qimg.convertToFormat(_mTARgetformat));
        if(_verbose)
            std::cout << _qimg.format() << " converted to: " << _tmpqimg.format() << std::endl;
    }
    if(_verbose) {
        std::cout << " Depth: " << _tmpqimg.depth()  << " bits"
                  << " Width:"  << _tmpqimg.width()
                  << " Height:" << _tmpqimg.height() << std::endl;
    }

    // QImage have one specific property - the scanline data is aligned on a 32-bit boundary
    // So I have found that if the line length is not divisible by 4,
    // extra bytes is added to the end of line to make length divisible by 4
    // Read more here: https://bugreports.qt.io/browse/QTBUG-68379?filter=-2
    // As we do not want to copy this extra bytes to IRPV::Image we should throw them out
    int _validbytesperline = _tmpqimg.width()*_tmpqimg.depth()/8;
    std::shared_ptr<uint8_t>_ptr(new uint8_t[_tmpqimg.height()*_validbytesperline], std::default_delete<uint8_t[]>());
    for(int i = 0; i < _tmpqimg.height(); ++i) {
        std::memcpy(_ptr.get()+i*_validbytesperline,_tmpqimg.constScanLine(i),_validbytesperline);
    }
    return IRPV::Image(_tmpqimg.width(),_tmpqimg.height(),_tmpqimg.depth(),_ptr);
}

//---------------------------------------------------

struct ROCPoint
{
    ROCPoint() {}
    double mTAR, mFAR, similarity;
};

//---------------------------------------------------

std::vector<ROCPoint> computeROC(uint _points, const std::vector<uint8_t> &_issameperson, size_t _totalpositive, size_t _totalnegative, const std::vector<double> &_similarity)
{
    std::vector<ROCPoint> _vROC(_points,ROCPoint());

    const double _maxsim = *std::max_element(_similarity.begin(), _similarity.end());
    const double _minsim = *std::min_element(_similarity.begin(), _similarity.end());
    const double _simstep = (_maxsim - _minsim)/_points;

    #pragma omp parallel for
    for(int i = 0; i < (int)_points; ++i) {
        const double _thresh = _minsim + i*_simstep;
        uint8_t _same;
        size_t  _truepositive = 0, _truenegative = 0;
        for(size_t j = 0; j < _similarity.size(); ++j) {
            _same = (_similarity[j] < _thresh) ? 0 : 1; // 1 - same, 0 - not the same
            if((_same == 1) && (_issameperson[j] == 1))
                _truepositive++;
            else if((_same == 0) && (_issameperson[j] == 0))
                _truenegative++;
        }
        _vROC[i].mTAR = static_cast<double>(_truepositive) / _totalpositive;
        _vROC[i].mFAR = 1.0 - static_cast<double>(_truenegative) / _totalnegative;
        _vROC[i].similarity = _thresh;
    }
    return _vROC;
}

//---------------------------------------------------

double findArea(const std::vector<ROCPoint> &_roc)
{
    double _area = 0;
    for(size_t i = 1; i < _roc.size(); ++i) {
        _area += (_roc[i-1].mFAR - _roc[i].mFAR)*(_roc[i-1].mTAR + _roc[i].mTAR)/2.0;
    }
    return _area;
}

//---------------------------------------------------

double findFRR(const std::vector<ROCPoint> &_roc, double _targetmFAR)
{
    for(size_t i = (_roc.size()-1); i >= 1; --i) { // not to 0 because of unsigned data type, we will hadle last value in final return
        if(_roc[i].mFAR > _targetmFAR)
            return 1.0 - _roc[i].mTAR;
    }
    return 1.0 - _roc[0].mTAR;
}

//---------------------------------------------------

QJsonArray serializeROC(const std::vector<ROCPoint> &_roc)
{
    QJsonArray _jsonarr;
    for(size_t i = 0; i < _roc.size(); ++i) {
        QJsonObject _jsonobj({
                                 qMakePair(QLatin1String("FAR"),QJsonValue(_roc[i].mFAR)),
                                 qMakePair(QLatin1String("TAR"),QJsonValue(_roc[i].mTAR)),
                                 qMakePair(QLatin1String("similarity"),QJsonValue(_roc[i].similarity))
                             });
        _jsonarr.push_back(qMove(_jsonobj));
    }
    return _jsonarr;
}

#endif // IRPVHELPER_H
