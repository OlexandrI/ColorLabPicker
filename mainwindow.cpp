#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"
#include "QDebug"
#include "QColor"
#include "QPainter"
#include <qmath.h>

#define HistoWidth 480
#define HistoHeight 320

#define getFileName(saveStr) QString(file).replace(QRegExp("(.*)\\.(\\w+)"), saveStr)

#define SaveMacro(source, suffix, format, quality) source.save(getFileName(QString("\\1_%1").arg(suffix)), format, quality);\
qDebug() << QString("Save %1-channel").arg(suffix)

#define SaveImgFile(source, suffix) source.save(getFileName(QString("\\1_%1.\\2").arg(suffix)));\
qDebug() << QString("Save %1-channel").arg(suffix)

#define SaveImgFileAndClear(source, suffix) SaveImgFile(source, suffix);\
source = QImage()

#define GenerateAndSave(generColor, suffix) tmp = QImage(img);\
for(int y = 0; y<tmp.height(); ++y){\
    for(int x = 0; x<tmp.width(); ++x){\
        QRgb point = tmp.pixel(x, y);\
        tmp.setPixel(x, y, generColor);\
    }\
}\
SaveImgFileAndClear(tmp, suffix)

#define DrawHisto(points, name, color, bg) points[257] = QPointF(HistoWidth, HistoHeight);\
do{\
QPixmap histoimg = QPixmap(HistoWidth, HistoHeight);\
QPainter p(&histoimg);\
p.setBrush(bg);\
p.drawRect(histoimg.rect());\
p.setBrush(color);\
p.drawPolygon(points, 258);\
SaveMacro(histoimg, QString("%1.png").arg(name), 0, 100);\
}while(0)

#define DrawHistoDef(points, name) DrawHisto(points, name, Qt::darkGray, Qt::lightGray)


QRgb Monochrome(int& V){
    return qRgb(V, V, V);
}
QRgb Monochrome(int V){
    return qRgb(V, V, V);
}
QRgb Sepia(QRgb c){
    int Red = (int)qMin(255.0, (qRed(c) * .393) + (qGreen(c) *.769) + (qBlue(c) * .189));
    int Green = (int)qMin(255.0, (qRed(c) * .349) + (qGreen(c) *.686) + (qBlue(c) * .168));
    int Blue = (int)qMin(255.0, (qRed(c) * .272) + (qGreen(c) *.534) + (qBlue(c) * .131));
    return qRgb(Red, Green, Blue);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

QPixmap MainWindow::render_colorpicker(QColor color, int gwidth, int gheight){
    QPixmap px(gwidth, gheight);
    QPainter painter(&px);
    painter.setBrush(Qt::lightGray);
    painter.drawRect(px.rect());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(gwidth/2,gheight/2);

    qreal hue = qMax(0.0, color.hsvHueF());
    qreal val = color.valueF();
    qreal sat = color.hsvSaturationF();
    qreal outer_radius = qMin(gwidth, gheight)/2;
    qreal inner_radius = outer_radius - 20;

    // hue wheel
    QPixmap hue_ring = QPixmap(outer_radius*2, outer_radius*2);
    {
        hue_ring.fill(Qt::transparent);
        QPainter painter(&hue_ring);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setCompositionMode(QPainter::CompositionMode_Source);

        const int hue_stops = 24;
        QConicalGradient gradient_hue(0, 0, 0);
        if ( gradient_hue.stops().size() < hue_stops )
        {
            for ( double a = 0; a < 1.0; a+=1.0/(hue_stops-1) )
            {
                gradient_hue.setColorAt(a, QColor::fromHsvF(a,1,1));
            }
            gradient_hue.setColorAt(1, QColor::fromHsvF(0,1,1));
        }

        painter.translate(outer_radius, outer_radius);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(gradient_hue));
        painter.drawEllipse(QPointF(0,0), outer_radius, outer_radius);

        painter.setBrush(Qt::transparent);//palette().background());
        painter.drawEllipse(QPointF(0,0), inner_radius,inner_radius);
    }

    painter.drawPixmap(-outer_radius, -outer_radius, hue_ring);

    // hue selector
    painter.setPen(QPen(Qt::black,3));
    painter.setBrush(Qt::NoBrush);
    QLineF ray(0, 0, outer_radius, 0);
    ray.setAngle(hue*360);
    QPointF h1 = ray.p2();
    ray.setLength(inner_radius);
    QPointF h2 = ray.p2();
    painter.drawLine(h1,h2);


    qreal triangle_side = inner_radius*qSqrt(3);
    // lum-sat square
    QImage inner_selector;
    {
        qreal side = triangle_side;
        qreal height = inner_radius*3/2;
        qreal ycenter = side/2;
        inner_selector = QImage(height, side, QImage::Format_RGB32);

        for (int x = 0; x < inner_selector.width(); x++ )
        {
            qreal pval = x / height;
            qreal slice_h = side * pval;
            for (int y = 0; y < inner_selector.height(); y++ )
            {
                qreal ymin = ycenter-slice_h/2;
                qreal psat = qBound(0.0,(y-ymin)/slice_h,1.0);

                inner_selector.setPixel(x,y, QColor::fromHsvF(hue, psat, pval, 1).rgb());
            }
        }
    }

    painter.rotate(-hue*360-60);
    painter.translate(QPointF(-inner_radius, -triangle_side/2));

    QPointF selector_position;
    {
        qreal side = triangle_side;
        qreal height = inner_radius*3/2;
        qreal slice_h = side * val;
        qreal ymin = side/2-slice_h/2;

        selector_position = QPointF(val*height, ymin + sat*slice_h);
        QPolygonF triangle;
        triangle.append(QPointF(0,side/2));
        triangle.append(QPointF(height,0));
        triangle.append(QPointF(height,side));
        QPainterPath clip;
        clip.addPolygon(triangle);
        painter.setClipPath(clip);
    }

    painter.drawImage(0,0, inner_selector);
    painter.setClipping(false);

    // lum-sat selector
    painter.setPen(QPen(val > 0.5 ? Qt::black : Qt::white, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(selector_position, 6, 6);

    return px;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->pushButton->setDisabled(true);
    file = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home", QString("*.jpg; *.bmp; *.png"));
    if(file.isEmpty()){
        QMessageBox msgBox(this);
        msgBox.setText("Please select a image!");
        msgBox.exec();
        ui->pushButton_2->setEnabled(false);
        ui->pushButton_3->setEnabled(false);
        ui->pushButton_4->setEnabled(false);
        return;
    }
   ui->pushButton->setDisabled(false);
   img = QImage(file);
   QImage img2 = img.scaled(ui->label->width(), ui->label->height());
   ui->label->setPixmap(QPixmap::fromImage(img2));
   ui->XCoord->setMaximum(img.width()-1);
   ui->YCoord->setMaximum(img.height()-1);
   ui->XCoord->setMinimum(0);
   ui->YCoord->setMinimum(0);
   ui->pushButton_2->setEnabled(true);
   ui->pushButton_3->setEnabled(true);
   ui->pushButton_4->setEnabled(true);
}

void MainWindow::on_pushButton_2_clicked()
{
    QColor point = QColor::fromRgb(img.pixel(ui->XCoord->value(), ui->YCoord->value()));
    ui->textBrowser->clear();
    QString res = QString("R: %1, G: %2, B: %3\r\nC: %4, M: %5, Y: %6, K: %7\r\nH: %8, S: %9, V: %10\r\nHEX: %11")
            .arg(point.red())
            .arg(point.green())
            .arg(point.blue())
            .arg(point.cyan())
            .arg(point.magenta())
            .arg(point.yellow())
            .arg(point.black())
            .arg(point.hue())
            .arg(point.saturation())
            .arg(point.value())
            .arg(point.name());
    ui->textBrowser->setPlainText(res);

    QPixmap triangle = render_colorpicker(point, ui->label_3->width(), ui->label_3->height());
    ui->label_3->setPixmap(triangle);
    SaveMacro(triangle, "triangle.png", 0, 100);
}

void MainWindow::on_pushButton_3_clicked()
{
    bool lowMemmory = (img.height() * img.width()) > 4194304;

    if(lowMemmory){
        QImage tmp;
        QString tmp_file;
        GenerateAndSave(qRgb(qRed(point), 0, 0), "red");
        GenerateAndSave(qRgb(0, qGreen(point), 0), "green");
        GenerateAndSave(qRgb(0, 0, qBlue(point)), "blue");
        GenerateAndSave(qRgb(qGray(point), qGray(point), qGray(point)), "whiteblack");
        GenerateAndSave(qRgb(QColor::fromRgb(point).hue(), QColor::fromRgb(point).hue(), QColor::fromRgb(point).hue()), "hue");
        GenerateAndSave(qRgb(QColor::fromRgb(point).saturation(), QColor::fromRgb(point).saturation(), QColor::fromRgb(point).saturation()), "saturation");
        GenerateAndSave(qRgb(QColor::fromRgb(point).value(), QColor::fromRgb(point).value(), QColor::fromRgb(point).value()), "value");
        GenerateAndSave(qRgb(qRed(point)>51?qRed(point):0, 0, qBlue(point)<127?qBlue(point):0), "levels");
        GenerateAndSave(Sepia(point), "sepia");
    }else{
        QImage red = QImage(img.width(), img.height(), img.format());
        QImage green = QImage(img.width(), img.height(), img.format());
        QImage blue = QImage(img.width(), img.height(), img.format());
        QImage wb = QImage(img.width(), img.height(), img.format());
        QImage hue = QImage(img.width(), img.height(), img.format());
        QImage satur = QImage(img.width(), img.height(), img.format());
        QImage value = QImage(img.width(), img.height(), img.format());
        QImage levels = QImage(img.width(), img.height(), img.format());
        QImage sepia = QImage(img.width(), img.height(), img.format());

        for(int y = 0; y<img.height(); ++y){
            for(int x = 0; x<img.width(); ++x){
                QColor point = QColor::fromRgb(img.pixel(x, y));
                red.setPixel(x, y, qRgb(point.red(), 0, 0));
                green.setPixel(x, y, qRgb(0, point.green(), 0));
                blue.setPixel(x, y, qRgb(0, 0, point.blue()));
                wb.setPixel(x, y, Monochrome(qGray(point.rgb())));
                hue.setPixel(x, y, Monochrome(point.hue()));
                satur.setPixel(x, y, Monochrome(point.saturation()));
                value.setPixel(x, y, Monochrome(point.value()));
                levels.setPixel(x, y, qRgb(point.red()>51?point.red():0, 0, point.blue()<127?point.blue():0));
                sepia.setPixel(x, y, Sepia(point.rgb()));
            }
        }
        QString tmp;
        QRegExp FRepl = QRegExp("(.*)\\.(\\w+)");
        SaveImgFileAndClear(red, "red");
        SaveImgFileAndClear(green, "green");
        SaveImgFileAndClear(blue, "blue");
        SaveImgFileAndClear(wb, "whiteblack");
        SaveImgFileAndClear(hue, "hue");
        SaveImgFileAndClear(satur, "saturation");
        SaveImgFileAndClear(value, "value");
        SaveImgFileAndClear(levels, "levels");
        SaveImgFileAndClear(sepia, "sepia");
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    // Calc for histogram
    img = img.convertToFormat(QImage::Format_RGB888);
    unsigned int histoR[257];
    unsigned int histoG[257];
    unsigned int histoB[257];
    unsigned int histoM[257];
    for(int q=0; q<257; ++q){
        histoR[q] = 0;
        histoG[q] = 0;
        histoB[q] = 0;
        histoM[q] = 0;
    }
    for(int y = 0; y<img.height(); ++y){
        for(int x = 0; x<img.width(); ++x){
            QRgb point = img.pixel(x, y);
            histoR[qRed(point)]++;
            histoG[qGreen(point)]++;
            histoB[qBlue(point)]++;
            histoM[(qRed(point) + qGreen(point) + qBlue(point)) / 3]++;
        }
    }
    for(int q=0; q<256; ++q){
        if(histoR[q]>histoR[256]) histoR[256] = histoR[q];
        if(histoG[q]>histoG[256]) histoG[256] = histoG[q];
        if(histoB[q]>histoB[256]) histoB[256] = histoB[q];
        if(histoM[q]>histoM[256]) histoM[256] = histoM[q];
    }
    unsigned int GMax = histoR[256];
    if(GMax < histoG[256]) GMax = histoG[256];
    if(GMax < histoB[256]) GMax = histoB[256];
    if(GMax < histoM[256]) GMax = histoM[256];

    // Draw Histograms
    QPointF pointsR[258];
    QPointF pointsG[258];
    QPointF pointsB[258];
    QPointF pointsM[258];
    pointsR[0] = QPointF(0.0, HistoHeight);
    pointsG[0] = QPointF(0.0, HistoHeight);
    pointsB[0] = QPointF(0.0, HistoHeight);
    pointsM[0] = QPointF(0.0, HistoHeight);
    float xStep = HistoWidth / 255.0;
    for(int q=1; q<=256; ++q){
        pointsR[q] = QPointF(xStep * float(q-1), (1.0 - float(histoR[q]) / float(histoR[256])) * float(HistoHeight));
        pointsG[q] = QPointF(xStep * float(q-1), (1.0 - float(histoG[q]) / float(histoG[256])) * float(HistoHeight));
        pointsB[q] = QPointF(xStep * float(q-1), (1.0 - float(histoB[q]) / float(histoB[256])) * float(HistoHeight));
        pointsM[q] = QPointF(xStep * float(q-1), (1.0 - float(histoM[q]) / float(histoM[256])) * float(HistoHeight));
    }
    DrawHistoDef(pointsR, "histoR");
    DrawHistoDef(pointsG, "histoG");
    DrawHistoDef(pointsB, "histoB");
    DrawHistoDef(pointsM, "histoM");

    QPixmap showHisto = QPixmap(HistoWidth, HistoHeight);\
    QPainter p(&showHisto);
    p.setBrush(Qt::lightGray);
    p.drawRect(showHisto.rect());
    p.setBrush(QColor(255, 0, 0, 64));
    p.drawPolygon(pointsR, 258);
    p.setBrush(QColor(0, 255, 0, 64));
    p.drawPolygon(pointsG, 258);
    p.setBrush(QColor(0, 0, 255, 64));
    p.drawPolygon(pointsB, 258);
    p.setBrush(QColor(64, 64, 64, 64));
    p.drawPolygon(pointsM, 258);
    QPixmap showHistoScaled = showHisto.scaled(ui->label_2->width(), ui->label_2->height());
    ui->label_2->setPixmap(showHistoScaled);
}




