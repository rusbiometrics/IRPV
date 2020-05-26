#include <iostream>

#include "irpvhelper.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    setlocale(LC_CTYPE,"Rus");
#endif
    // Default input values
    QDir indir, outdir;
    indir.setPath(""); outdir.setPath("");
    size_t vtpp = 1, etpp = 1, rocpoints = 512;
    bool verbose = false, rewriteoutput = false, shuffletemplates = false;
    QString apiresourcespath;
    QImage::Format qimgtargetformat = QImage::Format_RGB888; 
    // If no args passed, show help
    if(argc == 1) {
        std::cout << APP_NAME << " version " << APP_VERSION << std::endl;
        std::cout << "Options:" << std::endl
                  << "\t-g - force to open all images in 8-bit grayscale mode, if not set all images will be opened in 24-bit rgb color mode" << std::endl
                  << "\t-i - input directory with the images, note that this directory should have irpv-compliant structure" << std::endl
                  << "\t-o - output directory where result will be saved" << std::endl
                  << "\t-r - path where Vendor's API should search resources" << std::endl
                  << "\t-v - set how namy verification templates per person should be created (default: " << vtpp << ")" << std::endl
                  << "\t-e - set how namy enrollment templates per person should be created (default: " << etpp << ")" << std::endl
                  << "\t-p - set how many points for ROC curve should be computed (default: " << rocpoints << ")" << std::endl
                  << "\t-b - be more verbose (print all measurements)" << std::endl
                  << "\t-s - shuffle templates before matching" << std::endl
                  << "\t-w - force output file to be rewritten if already existed" << std::endl;
        return 0;
    }
    // Let's parse user's command input
    while((--argc > 0) && (**(++argv) == '-'))
        switch(*(++(*argv))) {
            case 'g':
                    qimgtargetformat = QImage::Format_Grayscale8;
                break;
            case 'i':
                    indir.setPath(QString(++(*argv)));
                break;
            case 'o':
                    outdir.setPath(QString(++(*argv)));
                break;
            case 'r':
                    apiresourcespath = QString(++(*argv));
                break;
            case 'v':
                    vtpp = QString(++(*argv)).toUInt();
                break;
            case 'e':
                    etpp = QString(++(*argv)).toUInt();
                break;
            case 'p':
                    rocpoints = QString(++(*argv)).toUInt();
                break;
            case 'b':
                    verbose = true;
                break;
            case 's':
                    shuffletemplates = true;
                break;
            case 'w':
                    rewriteoutput = true;
                break;
        }
    // Let's check if user have provided valid paths?
    if(indir.absolutePath().isEmpty()) {
        std::cerr << "Empty input directory path! Abort...";
        return 1;
    }
    if(outdir.absolutePath().isEmpty()) {
        std::cerr << "Empty output directory path! Abort...";
        return 2;
    }
    if(!indir.exists()) {
        std::cerr << "Input directory you've provided does not exists! Abort...";
        return 3;
    }
    if(!outdir.exists()) {       
        outdir.mkpath(outdir.absolutePath());
        if(!outdir.exists()) {
            std::cerr << "Can not create output directory in the path you've provided! Abort...";
            return 4;
        }
    }
    // Ok we can go forward
    std::cout << "Input dir:\t" << indir.absolutePath().toStdString() << std::endl;
    std::cout << "Output dir:\t" << outdir.absolutePath().toStdString() << std::endl;    
    // Let's also check if structure of the input directory is irpv-valid
    QDateTime startdt(QDateTime::currentDateTime());
    std::cout << std::endl << "Stage 1 - input directory parsing" << std::endl;
    QStringList subdirs = indir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
    std::cout << "  Total subdirs: " << subdirs.size() << std::endl;
    size_t validsubdirs = 0;
    QStringList filefilters;
    filefilters << "*.jpg" << "*.jpeg" << "*.gif" << "*.png" << ".bmp";
    const size_t minfilespp = (vtpp == 0 ? etpp : etpp + vtpp);
    for(int i = 0; i < subdirs.size(); ++i) {
        QStringList _files = QDir(indir.absolutePath().append("/%1").arg(subdirs.at(i))).entryList(filefilters,QDir::Files | QDir::NoDotAndDotDot);
        if(static_cast<uint>(_files.size()) >= minfilespp) {
            validsubdirs++;           
        }
    }
    std::cout << "  Valid subdirs: " << validsubdirs << std::endl;
    if(validsubdirs*etpp == 0) {
        std::cerr << std::endl << "There is 0 enrollment templates! Test could not be performed! Abort..." << std::endl;
        return 5;
    }
    QStringList distractorfiles = indir.entryList(filefilters,QDir::Files | QDir::NoDotAndDotDot);
    const size_t distractors = static_cast<size_t>(distractorfiles.size());
    std::cout << "  Distractor files: " << distractors << std::endl;
    if((validsubdirs*vtpp + distractors) == 0) {
        std::cerr << std::endl << "There is 0 verification templates! Test could not be performed! Abort..." << std::endl;
        return 6;
    }

    QElapsedTimer elapsedtimer;
    // Let's try to init Vendor's API
    std::cout << std::endl << "Stage 2 - Vendor's API loading" << std::endl;    
    std::shared_ptr<IRPV::VerifInterface> recognizer = IRPV::VerifInterface::getImplementation();
    std::cout << "  Initializing: ";
    elapsedtimer.start();
    IRPV::ReturnStatus status = recognizer->initialize(apiresourcespath.toStdString());
    qint64 inittimems = elapsedtimer.elapsed();
    std::cout << status.code << std::endl;
    std::cout << "  Time: " << inittimems << " ms" << std::endl;
    if(status.code != IRPV::ReturnCode::Success) {
        std::cout << "Vendor's error description: " << status.info << std::endl;
        std::cout << "Can not initialize Vendor's API! Abort..." << std::endl;
        return 7;
    }

    // We need also check if output file does not exist
    QFile outputfile(outdir.absolutePath().append("/%1.json").arg(VENDOR_API_NAME));
    if(outputfile.exists() && (rewriteoutput == false)) {
        std::cerr << "Output file already exists in the target location! Abort...";
        return 8;
    } else if(outputfile.open(QFile::WriteOnly) == false) { // and could be opened
        std::cerr << "Can not open output file for write! Abort...";
        return 9;
    }

    // Seems that all requirements are matched to test, let's start files processing
    std::cout << std::endl << "Stage 3 - templates generation" << std::endl;

    std::vector<BiometricTemplate> etemplates; // here we will store enrollment templates
    etemplates.resize(validsubdirs*etpp);
    size_t   etpos = 0;     // position in etemplates
    double etgentime = 0; // enrollment template gen time holder
    size_t   eterrors = 0;  // enrollment template gen errors

    std::vector<BiometricTemplate> vtemplates; // here we will store verification templates
    vtemplates.resize(validsubdirs*vtpp + distractors);
    size_t   vtpos = 0;     // position in vtemplates
    double vtgentime = 0; // verification templates gen time holder
    size_t   vterrors = 0;  // verification template gen errors

    size_t label = 0;

    IRPV::Image irpvimg;
    for(int i = 0; i < subdirs.size(); ++i) {
        QDir _subdir(indir.absolutePath().append("/%1").arg(subdirs.at(i)));
        QStringList _files = _subdir.entryList(filefilters,QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        if(static_cast<size_t>(_files.size()) >= minfilespp) {

            std::cout << std::endl << "  Label: " << label << " - " << subdirs.at(i) << std::endl;

            for(size_t j = 0; j < etpp; ++j) {
                if(verbose)
                    std::cout << "   - enrollment template: " << _files.at(j) << std::endl;
                std::vector<uint8_t> _templ;
                irpvimg = readimage(_subdir.absoluteFilePath(_files.at(j)),qimgtargetformat,verbose);
                elapsedtimer.start();
                status = recognizer->createTemplate(irpvimg,IRPV::TemplateRole::Enrollment_11,_templ);
                etgentime += elapsedtimer.nsecsElapsed();
                etemplates[etpos++] = BiometricTemplate(label,IRPV::TemplateRole::Enrollment_11,std::move(_templ));
                if(status.code != IRPV::ReturnCode::Success) {
                    eterrors++;
                    if(verbose) {
                        std::cout << "   " << status.code << std::endl;
                        std::cout << "   " << status.info << std::endl;
                    }
                }
            }

            for(size_t j = etpp; j < minfilespp; j++) {
                if(verbose)
                    std::cout << "   - verification template: " << _files.at(j) << std::endl;
                std::vector<uint8_t> _templ;
                irpvimg = readimage(_subdir.absoluteFilePath(_files.at(j)),qimgtargetformat,verbose);
                elapsedtimer.start();                
                status = recognizer->createTemplate(irpvimg,IRPV::TemplateRole::Verification_11,_templ);
                vtgentime += elapsedtimer.nsecsElapsed();
                vtemplates[vtpos++] = BiometricTemplate(label,IRPV::TemplateRole::Verification_11,std::move(_templ));
                if(status.code != IRPV::ReturnCode::Success) {
                    vterrors++;
                    if(verbose) {
                        std::cout << "   " << status.code << std::endl;
                        std::cout << "   " << status.info << std::endl;
                    }
                }
            }

            label++; // increment for the next person / subdir
        }
    }
    // Also we need to enroll all distractors
    for(int i = 0; i < distractorfiles.size(); ++i) {
        std::cout << std::endl << "  Label(D): " << label << " - " << distractorfiles.at(i) << std::endl;
        std::vector<uint8_t> _templ;
        irpvimg = readimage(indir.absoluteFilePath(distractorfiles.at(i)),qimgtargetformat,verbose);
        elapsedtimer.start();
        status = recognizer->createTemplate(irpvimg,IRPV::TemplateRole::Verification_11,_templ);
        vtgentime += elapsedtimer.nsecsElapsed();
        vtemplates[vtpos++] = BiometricTemplate(label,IRPV::TemplateRole::Verification_11,std::move(_templ));
        if(status.code != IRPV::ReturnCode::Success) {
            vterrors++;
            if(verbose) {
                std::cout << "   " << status.code << std::endl;
                std::cout << "   " << status.info << std::endl;
            }
        }
        label++; // increment for the next distractor
    }

    etgentime /= etemplates.size();
    vtgentime /= vtemplates.size();

    std::cout << "\nEnrollment templates" << std::endl
              << "  Total: " << etemplates.size() << std::endl
              << "  Errors:  " << eterrors << std::endl
              << "  Avgtime: " << 1e-6 * etgentime << " ms" << std::endl
              << "\nVerification templates" << std::endl
              << "  Total: " << vtemplates.size() << std::endl
              << "  Errors:  " << vterrors << std::endl
              << "  Avgtime: " << 1e-6 * vtgentime << " ms" << std::endl;

    // Optional shuffle enrollment templates to prevent attacks on system
    if(shuffletemplates) {
        std::srand ( unsigned ( std::time(0) ) );
        std::cout << std::endl << "Shuffling templates" << std::endl;
        std::random_shuffle(etemplates.begin(),etemplates.end());
    }

    // Ok, templates are ready, so we can start to match them
    std::cout << std::endl << "Stage 4 - templates match" << std::endl;

    size_t comparisions = etemplates.size()*vtemplates.size();
    size_t totalpositivepairs = etpp*vtpp*validsubdirs;
    size_t totalnegativepairs = etpp*vtpp*validsubdirs*(validsubdirs-1) + etpp*validsubdirs*distractors;
    std::vector<double>  similarities(comparisions,0); // here we will store similarity
    std::vector<uint8_t> issameperson(comparisions,0); // 1 - same, 0 - not the same, init by 0 because the number of true negative pairs is greater than true positive
    double matchtime = 0;

    size_t mterrors = 0;

    size_t matchcounter = 0;
    for(size_t i = 0; i < etemplates.size(); ++i) {
        std::cout << "  Matching for label: " << etemplates[i].label << std::endl;
        for(size_t j = 0; j < vtemplates.size(); ++j) {
            elapsedtimer.start();
            status = recognizer->matchTemplates(vtemplates[j].data,etemplates[i].data,similarities[matchcounter]);
            matchtime += elapsedtimer.nsecsElapsed();
            if(etemplates[i].label == vtemplates[j].label) {
                issameperson[matchcounter] = 1;
            }
            if(status.code != IRPV::ReturnCode::Success) {
                mterrors++;
                if(verbose) {
                    std::cout << "   " << status.code << std::endl;
                    std::cout << "   " << status.info << std::endl;
                }
            }
            matchcounter++;
        }
    }

    std::cout << std::endl << "  Total comparisions: " << comparisions << std::endl;
    std::cout << "  Positive pairs: " << totalpositivepairs << std::endl;
    std::cout << "  Negative pairs: " << totalnegativepairs << std::endl;
    std::cout << "  Errors: " << mterrors << std::endl;
    matchtime /= comparisions;
    std::cout << std::endl << "Avg match time: " << matchtime*1e-3 << " us" << std::endl;


    // Ok, now we can compute ROC table
    std::cout << std::endl << "Stage 5 - ROC computation" << std::endl;

    // But first let's release unused memory
    size_t etsizebytes = etemplates[0].data.size();
    etemplates.clear(); etemplates.shrink_to_fit();
    size_t vtsizebytes = vtemplates[0].data.size();
    vtemplates.clear(); vtemplates.shrink_to_fit();

    std::vector<ROCPoint> vROC = computeROC(rocpoints, issameperson, totalpositivepairs, totalnegativepairs, similarities);

    double rocarea = findArea(vROC);
    std::cout << "  Area under the ROC curve: " << rocarea << std::endl;
    // We also can find FRR (FAR) according to test data
    const uint confexaples = 10; // how many exaples are needed to count result confident
    // First let's estimate the best FAR value we can select
    double bestFAR = std::exp(-std::log(10.0) * std::floor(std::log10( static_cast<double>(totalnegativepairs) / confexaples)));
    if(bestFAR > 1.0)
        bestFAR = 1.0;
    // Ok now we can find FRR for selected FAR from the ROC curve
    double bestFRR = findFRR(vROC, bestFAR);
    // But the value of the FRR we have found could be not confident, so additional check is needed
    if(bestFRR < (static_cast<double>(confexaples) / totalpositivepairs)) {
        if((static_cast<double>(confexaples) / totalpositivepairs) < 1.0)
            bestFRR = static_cast<double>(confexaples) / totalpositivepairs;
        else
            bestFRR = 1.0;
    }
    std::cout << "  Best FRR (FAR): " << bestFRR << " (" << bestFAR << ")" << std::endl;
    QDateTime enddt = QDateTime::currentDateTime();

    // Let's print time consumption
    showTimeConsumption(startdt.secsTo(enddt));
    // In the end we need to serialize test data
    std::cout << " Wait untill output data will be saved..." << std::endl;

    QJsonObject etjsobj({
                            qMakePair(QLatin1String("Templates"),QJsonValue(static_cast<qint64>(etpp*validsubdirs))),
                            qMakePair(QLatin1String("Perperson"),QJsonValue(static_cast<int>(etpp))),
                            qMakePair(QLatin1String("Errors"),QJsonValue(static_cast<qint64>(eterrors))),
                            qMakePair(QLatin1String("Gentime_ms"),QJsonValue(etgentime*1e-6)),
                            qMakePair(QLatin1String("Size_bytes"),QJsonValue(static_cast<qint64>(etsizebytes)))
                        });

    QJsonObject vtjsobj({
                            qMakePair(QLatin1String("Templates"),QJsonValue(static_cast<qint64>(vtpp*validsubdirs))),
                            qMakePair(QLatin1String("Perperson"),QJsonValue(static_cast<int>(vtpp))),
                            qMakePair(QLatin1String("Distractors"),QJsonValue(static_cast<qint64>(distractors))),
                            qMakePair(QLatin1String("Errors"),QJsonValue(static_cast<qint64>(vterrors))),
                            qMakePair(QLatin1String("Gentime_ms"),QJsonValue(vtgentime*1e-6)),
                            qMakePair(QLatin1String("Size_bytes"),QJsonValue(static_cast<qint64>(vtsizebytes)))
                        });

    QJsonObject matchjsobj({
                               qMakePair(QLatin1String("Positivepairs"), QJsonValue(static_cast<qint64>(totalpositivepairs))),
                               qMakePair(QLatin1String("Negativepairs"), QJsonValue(static_cast<qint64>(totalnegativepairs))),
                               qMakePair(QLatin1String("Errors"), QJsonValue(static_cast<qint64>(mterrors))),
                               qMakePair(QLatin1String("Matchtime_us"), QJsonValue(matchtime*1e-3))
                           });

    QJsonObject jsonobj({
                            qMakePair(QLatin1String("Name"),QString(VENDOR_API_NAME)),
                            qMakePair(QLatin1String("StartDT"),startdt.toString("dd.MM.yyyy hh:mm:ss")),
                            qMakePair(QLatin1String("EndDT"),enddt.toString("dd.MM.yyyy hh:mm:ss")),
                            qMakePair(QLatin1String("Enrollment"),etjsobj),
                            qMakePair(QLatin1String("Verification"),vtjsobj),
                            qMakePair(QLatin1String("Match"),matchjsobj),
                            qMakePair(QLatin1String("ROC"),serializeROC(vROC)),
                            qMakePair(QLatin1String("ROCarea"),QJsonValue(rocarea)),
                            qMakePair(QLatin1String("FAR"),QJsonValue(bestFAR)),
                            qMakePair(QLatin1String("FRR"),QJsonValue(bestFRR)),
                            qMakePair(QLatin1String("Initms"),inittimems)
                        });

    outputfile.write(QJsonDocument(jsonobj).toJson());
    outputfile.close();
    std::cout << " Data saved" << std::endl;
    return 0;
}
