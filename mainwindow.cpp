#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/photo.hpp"
#include <math.h>
#include <iostream>
#include <QMessageBox>
#include <QInputDialog>
#include <fstream>
#include <time.h>
#include <QtCore/QCoreApplication>
#include <QMovie>
#include <algorithm>    // std::reverse
#include <vector>       // std::vector

using namespace cv;
using namespace std;

const double amin = 1000;
const double amax = 10000000000;
const float cosAngle = 0.3;

Mat thresh,image;
String imagePath;

float realAreaSquare;
bool area,sumareas,wid,len,widlen,avedev,per;

vector<vector<Point> > square;
vector<vector<Point> > leaves;
vector<vector<Point> > leavesPCA;
vector<double> leavesPer;
vector<double> leavesArea;

QString result = "",auxExport = "",auxExport2 = "";
QString remover,spec,treat,rep;

static QSqlDatabase database;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
        ui->setupUi(this);

        QString dir = qApp->applicationDirPath();
        QString dirbanco = dir+"/bd/bd.db";

        QString dirdriver = dir+"/sqldrivers/";
        QCoreApplication::addLibraryPath(dirdriver);

        database = QSqlDatabase::addDatabase("QSQLITE");
        database.setDatabaseName(dirbanco);
        if(!database.open()){
            ui->exibirHist->setText("Unable to connect to bank.");
            ui->scrollHist->setWidget(ui->exibirHist);
            ui->scrollHist->setAlignment(Qt::AlignHCenter);
        }

        //Visivel só depois que calcular as dimensoes
        ui->labelResult->setVisible(0);
        ui->scrollResult->setVisible(0);
        ui->btnExport->setVisible(0);
        ui->btnRemove->setVisible(0);
    }

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_btnImagem_clicked(){
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Images(*.png *.jpeg *.jpg)"));
    dialog.setViewMode(QFileDialog::Detail);
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Imagens"), "",tr("Image Files (*.png *.jpeg *.jpg"));

    if(!fileName.isEmpty()){
        ui->endImagem->setText(fileName);
        imagePath = ui->endImagem->text().toStdString();
        image = imread(imagePath, IMREAD_COLOR );
        QImage imageaux= QImage((uchar*) image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        ui->exibirImagem->setPixmap(QPixmap:: fromImage(imageaux).scaled((451*imageaux.width())/imageaux.height(),451));
    }
}

Point2f GetPointAfterRotate(Point2f inputpoint,Point2f center,double angle){
    Point2d preturn;
    preturn.x = (inputpoint.x - center.x)*cos(-angle) - (inputpoint.y - center.y)*sin(-angle)+center.x;
    preturn.y = (inputpoint.x - center.x)*sin(-angle) + (inputpoint.y - center.y)*cos(-angle)+center.y;
    return preturn;
}

/*Point GetPointAfterRotate(Point inputpoint,Point center,double angle){
    Point preturn;
    preturn.x = (inputpoint.x - center.x)*cos(-1*angle) - (inputpoint.y - center.y)*sin(-1*angle)+center.x;
    preturn.y = (inputpoint.x - center.x)*sin(-1*angle) + (inputpoint.y - center.y)*cos(-1*angle)+center.y;
    return preturn;
}*/

double getOrientation(vector<Point> &pts, Point2f& pos){
    //Construct a buffer used by the pca analysis
    Mat data_pts = Mat(pts.size(), 2, CV_64FC1);
    for (int i = 0; i < data_pts.rows; ++i){
        data_pts.at<double>(i, 0) = pts[i].x;
        data_pts.at<double>(i, 1) = pts[i].y;
    }

    //Perform PCA analysis
    PCA pca_analysis(data_pts, Mat(), PCA::DATA_AS_ROW);

    //Store the position of the object
    pos = Point2f(pca_analysis.mean.at<double>(0, 0),
    pca_analysis.mean.at<double>(0, 1));

    //Store the eigenvalues and eigenvectors
    vector<Point2d> eigen_vecs(2);
    vector<double> eigen_val(2);
    for (int i = 0; i < 2; ++i){
        eigen_vecs[i] = Point2d(pca_analysis.eigenvectors.at<double>(i, 0),
        pca_analysis.eigenvectors.at<double>(i, 1));
        eigen_val[i] = pca_analysis.eigenvalues.at<double>(i,0);
    }
    return atan2(eigen_vecs[0].y, eigen_vecs[0].x);
}

static double cosineAngle( Point pt1, Point pt2, Point pt0 ){
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

void pca(vector<vector<Point> >& contours, int i){
    Point2f* pos = new Point2f();
    double dOrient =  getOrientation(contours[i], *pos);
    /*int xmin = 99999;
    int xmax = 0;
    int ymin = 99999;
    int ymax = 0;*/

    for (size_t j = 0;j<contours[i].size();j++){
        contours[i][j] = GetPointAfterRotate(contours[i][j],(Point)*pos,dOrient);
        /*if (contours[i][j].x < xmin)
            xmin = contours[i][j].x;
        if (contours[i][j].x > xmax)
            xmax = contours[i][j].x;
        if (contours[i][j].y < ymin)
            ymin = contours[i][j].y;
        if (contours[i][j].y > ymax)
            ymax = contours[i][j].y;*/
     }
}

static void findObjects(){
    Mat gray;
    vector<vector<Point> > contours;
    vector<Point> approx;

    cvtColor( image, gray, COLOR_BGR2GRAY );
    threshold( gray, thresh, 60, 255, THRESH_BINARY_INV|THRESH_OTSU);
    findContours(thresh, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    for(size_t i = 0; i < contours.size(); i++){
        double auxper = arcLength(contours[i], true);
        approxPolyDP(contours[i], approx, auxper*0.02, true);
        double auxarea = fabs(contourArea(contours[i]));

        if(approx.size() == 4 && auxarea > amin && auxarea < amax && isContourConvex(approx)){
            double maxCosine = 0;

            for(int j = 2; j < 5; j++){
                double cosine = fabs(cosineAngle(approx[j%4], approx[j-2], approx[j-1]));
                maxCosine = MAX(maxCosine, cosine);
             }

             if(maxCosine < cosAngle){
                 square.push_back(contours[i]);
             }
             else{
                 leaves.push_back(contours[i]); //Usado para desenhar os contornos originais
                 pca(contours,i);
                 leavesPCA.push_back(contours[i]); //Usado nos cálculos
                 leavesPer.push_back(auxper);
                 leavesArea.push_back(auxarea);
             }
        }else if(auxarea > amin && auxarea < amax){
            leaves.push_back(contours[i]);
            pca(contours,i);
            leavesPCA.push_back(contours[i]);
            leavesPer.push_back(auxper);
            leavesArea.push_back(auxarea);
        }
    }
}

static void surfaceCalc(){
    result.clear();
    auxExport.clear();
    auxExport2.clear();

    //---------------------Variaveis auxiliares calculos-----------------------
    float widSquare=0, lenSquare=0, sum=0.0;
    float aveWidth=0.0, aveLength=0.0, aveArea=0.0, avePerimeter=0.0; //average
    float stdWidth=0.0, stdLength=0.0, stdArea=0.0, stdPerimeter=0.0; //standard deviation
    int size = leaves.size(); //tamanho vetores e quantidade de folhas..
    float Width[size], Length[size], Area[size], Perimeter[size], WidLen[size];
    float pixelsAreaSquare=0.0, realPerSquare=0.0, pixelsPerSquare=0.0;

    //---------------------Variaveis auxiliares banco-----------------------
    QSqlQuery query;
    QString nome = QString::fromStdString(imagePath);
    nome = nome.split("/")[nome.split("/").size()-1];
    char dateStr [9]; _strdate(dateStr); char timeStr [9]; _strtime(timeStr);
    QString id_Imagem = dateStr; id_Imagem+=" "; id_Imagem+= timeStr;


    //-------------------SQUARE----------------------

    //Calculo dimensoes quadrado
     pixelsPerSquare = arcLength(square[0],true);

    if(wid || len){
        widSquare = ( pixelsPerSquare)/4;
        lenSquare = widSquare;
    }

    if(area) pixelsAreaSquare = contourArea(square[0]);

    if(per) realPerSquare = sqrt(float(realAreaSquare))* 4;

    //Desenhar quadrado
    const Point* p = &square[0][0];
    int n = (int)square[0].size();
    polylines(image, &p, &n, 1, true, Scalar(0,255,0), 10, LINE_AA);

    //-------------Calculo das dimensoes----------------
    vector<Rect> boundRect(size);

    for(int i = 0; i < size; i++ ){

        //Desenhar e numerar folhas
        const Point* p = &leaves[i][0];
        int n = (int)leaves[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,0,255), 10, LINE_AA);
        putText(image, to_string(i+1),leaves[i].at(leaves[i].capacity()/2), FONT_HERSHEY_DUPLEX, 4, Scalar(255,0,0), 12);

        result.append("\nLeaf: "); result.append(QString::number(i+1));
        result.append("\n\n");

        //_____________Calculo Largura e Comprimento_____________
        if(wid || len){
            boundRect[i] = boundingRect(leavesPCA[i]);

            float realSideSquare = sqrt(float(realAreaSquare));

            //float aux = sqrt((pow((boundRect[i].tl().x - boundRect[i].tl().x),2)+pow((boundRect[i].br().y - boundRect[i].tl().y),2)));
            float aux = boundRect[i].width;
            aux = (aux * realSideSquare)/widSquare;

            //float aux2 = sqrt((pow((boundRect[i].tl().x  - boundRect[i].br().x),2)+pow((boundRect[i].tl().y - boundRect[i].tl().y),2)));
            float aux2 = boundRect[i].height;
            aux2 = (aux2 * realSideSquare)/lenSquare;

            if(aux2 > aux){
                 if(avedev && wid) aveWidth += aux;
                 if(avedev && len) aveLength += aux2;
                 if(wid){
                     result.append("\nWidth: "); result.append(QString::number(aux));
                     Width[i] = aux;
                 }
                 if(len){
                     result.append("\nLength: "); result.append(QString::number(aux2));
                     Length[i] = aux2;
                 }

                 if(widlen){
                     result.append("\nWidth/Length: "); result.append(QString::number(aux/aux2));
                     WidLen[i] = aux/aux2;
                 }

            }else{
                 if(avedev && wid) aveWidth += aux2;
                 if(avedev && len) aveLength += aux;
                 if(wid){
                     result.append("\nWidth: "); result.append(QString::number(aux2));
                      Width[i] = aux2;
                 }
                 if(len){
                     result.append("\nLength: "); result.append(QString::number(aux));
                     Length[i] = aux;
                 }
                 if(widlen){
                     result.append("\nWidth/Length: "); result.append(QString::number(aux2/aux));
                     WidLen[i] = aux2/aux;
                 }
             }
        }


        //_____________Calculo Area_____________
        if(area){
            float auxArea = ((leavesArea[i] * realAreaSquare)/pixelsAreaSquare);
            if(sumareas) sum += auxArea;
            result.append("\nArea: "); result.append(QString::number(auxArea));
            if(avedev && area) aveArea += auxArea;
            Area[i] = auxArea;
        }

        //_____________Calculo Perimetro_____________
        if(per){
            float auxPer = ((leavesPer[i] * realPerSquare)/pixelsPerSquare);
            result.append("\nPerimeter: "); result.append(QString::number(auxPer));
            if(avedev && per) avePerimeter += auxPer;
            Perimeter[i] = auxPer;
        }

        result.append("\n\n");

        //_____________Export_____________
        if(wid && len && widlen && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(WidLen[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(wid && len && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(WidLen[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(WidLen[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(WidLen[i])); auxExport2.append("\n");
        }
        else if(wid && len && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append("\n");
        }
        else if(wid && len && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(wid && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(wid && len){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append("\n");
        }
        else if(wid && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append("\n");
        }
        else if(wid && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(len && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(len && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append("\n");
        }
        else if(len && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(len){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Length[i])); auxExport2.append("\n");
        }
        else if(wid){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Width[i])); auxExport2.append("\n");
        }
        else if(area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
        else if(area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Area[i])); auxExport2.append("\n");
        }
        else if(per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(Perimeter[i])); auxExport2.append("\n");
        }
    } 


    //_____________Result sum areas_____________
    if(sumareas){
        result.append("\nSum areas: "); result.append(QString::number(sum));
        result.append("\n\n");
    }

    //_____________Calculo Media e Desvio_____________
    if(avedev){
        //_____________Media_____________
        if(avedev && wid){
            aveWidth = aveWidth / size;
            result.append("\nAverage width: "); result.append(QString::number(aveWidth));
        }

        if(avedev && len){
            aveLength = aveLength / size;
            result.append("\nAverage lenght: "); result.append(QString::number(aveLength));
        }

        if(avedev && area){
            aveArea = aveArea / size;
            result.append("\nAverage area: "); result.append(QString::number(aveArea));
        }

        if(avedev && per){
            avePerimeter = avePerimeter / size;
            result.append("\nAverage perimeter: "); result.append(QString::number(avePerimeter));
        }

         result.append("\n\n");

        //_____________Desvio_____________

        if(avedev && wid){
            for(int i = 0; i < size; i++ ){
                stdWidth += pow(Width[i]-aveWidth,2);
            }
            stdWidth = sqrt(stdWidth / size);
            result.append("\nWidth deviation: "); result.append(QString::number(stdWidth));
        }

        if(avedev && len){
            for(int i = 0; i < size; i++ ){
                stdLength += pow(Length[i]-aveLength,2);
            }
            stdLength = sqrt(stdLength / size);
            result.append("\nLenght deviation: "); result.append(QString::number(stdLength));
        }

        if(avedev && area){
            for(int i = 0; i < size; i++ ){
                stdArea += pow(Area[i]-aveArea,2);
            }
            stdArea = sqrt(stdArea / size);
            result.append("\nArea deviation: "); result.append(QString::number(stdArea));
        }

        if(avedev && per){
            for(int i = 0; i < size; i++ ){
                stdPerimeter += pow(Perimeter[i]-avePerimeter,2);
            }
            stdPerimeter = sqrt(stdPerimeter / size);
            result.append("\nPerimeter deviation: "); result.append(QString::number(stdPerimeter));
        }

         result.append("\n\n");
     }

    //_____________Inserindo Media e Desvio no banco e no export_____________


    if(avedev && area && wid && len && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+
                      QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");

        auxExport2.append("Sum:,,,");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && len && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+
                      QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");

        auxExport2.append("Sum:,,,");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && len && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && len && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && area && wid && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +","+QString::number(aveArea)+","+QString::number(stdArea)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");
    }
    else if(avedev && wid && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && wid && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveLength)+","+QString::number(stdLength)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveLength));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdLength));auxExport2.append("\n");
    }
    else if(avedev && area && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio,sumareas,"
                      "per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveArea)
                      +","+QString::number(stdArea)+","+QString::number(sum)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");

        auxExport2.append("Sum:,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveArea)
                      +","+QString::number(stdArea)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");

        auxExport2.append("Sum:,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio,per_Media,per_desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveArea)+","+QString::number(stdArea)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && area && wid){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+","+QString::number(aveArea)+","+QString::number(stdArea)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");
    }
    else if(avedev && area && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveLength)
                      +","+QString::number(stdLength)+","+QString::number(aveArea)+","+QString::number(stdArea)+","
                      +QString::number(avePerimeter)+","+QString::number(stdPerimeter)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && area && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveLength)
                      +","+QString::number(stdLength)+","+QString::number(aveArea)+","+QString::number(stdArea)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");
    }
    else if(avedev && area && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,area_Media,area_Desvio,per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+
                      QString::number(realAreaSquare)+","+QString::number(aveArea)+","+
                      QString::number(stdArea)+","+QString::number(avePerimeter)+","+QString::number(stdPerimeter)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveArea));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdArea)); auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && area){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveArea)+","+QString::number(stdArea)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveArea));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdArea)); auxExport2.append("\n");
    }
    else if(avedev && wid && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,larg_Media,larg_Desvio, per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+
                      QString::number(realAreaSquare)+","+QString::number(aveWidth)+","+
                      QString::number(stdWidth)+","+QString::number(avePerimeter)+","+QString::number(stdPerimeter)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && wid){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveWidth)
                      +","+QString::number(stdWidth)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveWidth));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdWidth));auxExport2.append("\n");
    }
    else if(avedev && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,comp_Medio,comp_Desvio, per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+
                      QString::number(realAreaSquare)+","+QString::number(aveLength)+","+
                      QString::number(stdLength)+","+QString::number(avePerimeter)+","+QString::number(stdPerimeter)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append(",");
        auxExport2.append(QString::number(avePerimeter));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append(",");
        auxExport2.append(QString::number(stdPerimeter));auxExport2.append("\n");
    }
    else if(avedev && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,comp_Medio,comp_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(aveLength)+","+QString::number(stdLength)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(aveLength));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(stdLength));auxExport2.append("\n");
    }
    else if(area && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+","+QString::number(sum)+")");
        query.exec();

        if(area && sumareas && wid && len && widlen){
            auxExport2.append("Sum:,,,,"); auxExport2.append(QString::number(sum));
        }
        else if(area && sumareas && wid && len){
            auxExport2.append("Sum:,,,"); auxExport2.append(QString::number(sum));
        }
        else if(area && sumareas && wid){
            auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
        }
        else if(area && sumareas && len){
            auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
        }
        else{
            auxExport2.append("Sum:,"); auxExport2.append(QString::number(sum));
        }


    }
    else if(area || wid || len || per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad) values ('"+id_Imagem+
                      "','"+nome+"','"+spec+"','"+treat+"','"+rep+"',"+QString::number(realAreaSquare)+")");
        query.exec();
    }

    //_____________Inserindo Area,Largura,Comprimento e Perimetro no banco_____________

    for(int i = 0; i < size; i++ ){
        if(wid && len && area && widlen && per){
                    query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,"
                                  "largcomp,per) values ("+QString::number((i+1))+",'"+
                                  id_Imagem+"',"+QString::number(Area[i])+","+
                                  QString::number(Width[i])+","+QString::number(Length[i])+","+
                                  QString::number(WidLen[i])+","+QString::number(Perimeter[i])+")");
                    query.exec();
                }
        else if(wid && len && area && widlen){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,largcomp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Width[i])+","+QString::number(Length[i])+","+QString::number(WidLen[i])+")");
            query.exec();
        }
        else if(wid && len && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Width[i])+","+QString::number(Length[i])+","+
                          QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(wid && len && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Width[i])+","+QString::number(Length[i])+")");
            query.exec();
        }
        else if(wid && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Width[i])+","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(wid && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Width[i])+")");
            query.exec();
        }
        else if(len && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Length[i])+","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(len && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Length[i])+")");
            query.exec();
        }
        else if(area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Area[i])
                          +")");
            query.exec();
        }
        else if(wid && len && widlen && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,largcomp,per)"
                          " values ("+QString::number((i+1))+",'"+id_Imagem+"',"+
                          QString::number(Width[i])+","+QString::number(Length[i])+","+
                          QString::number(WidLen[i])+","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(wid && len && widlen){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,largcomp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Width[i])+","
                          +QString::number(Length[i])+","+QString::number(WidLen[i])+")");
            query.exec();
        }
        else if(wid && len && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Width[i])+","
                          +QString::number(Length[i])+","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(wid && len){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Width[i])+","
                          +QString::number(Length[i])+")");
            query.exec();
        }
        else if(wid && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+
                          QString::number( Width[i])+","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(wid){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Width[i])+")");
            query.exec();
        }
        else if(len && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Length[i])
                          +","+QString::number(Perimeter[i])+")");
            query.exec();
        }
        else if(len){
            query.prepare("insert into folha(num_Folha,id_Imagem,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Length[i])+")");
            query.exec();
        }
        else if(per){
            query.prepare("insert into folha(num_Folha,id_Imagem,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(Perimeter[i])+")");
            query.exec();
        }
    }

    //_____________Export_____________

    auxExport.append("Image:," + nome);
    auxExport.append("\nSpecies:," + spec);
    auxExport.append("\nTreatment:," + treat);
    auxExport.append("\nReplicate:," + rep);
    auxExport.append("\nScale pattern area:,"); auxExport.append(QString::number(realAreaSquare));
    auxExport.append("\nNumber of leaves:,"); auxExport.append(QString::number(size));
    auxExport.append("\n\n");

    if(wid && len && widlen && area && per) auxExport.append("Number of leaf,Width,Length,Width/Length,Area,Perimeter\n\n");
    else if(wid && len && area && per) auxExport.append("Number of leaf,Width,Length,Area,Perimeter\n\n");
    else if(wid && len && widlen && area) auxExport.append("Number of leaf,Width,Length,Width/Length,Area\n\n");
    else if(wid && len && widlen && per) auxExport.append("Number of leaf,Width,Length,Width/Length,Perimeter\n\n");
    else if(wid && len && widlen) auxExport.append("Number of leaf,Width,Length,Width/Length\n\n");
    else if(wid && len && area) auxExport.append("Number of leaf,Width,Length,Area\n\n");
    else if(wid && len && per) auxExport.append("Number of leaf,Width,Length,Perimeter\n\n");
    else if(wid && len) auxExport.append("Number of leaf,Width,Length\n\n");
    else if(wid && area && per) auxExport.append("Number of leaf,Width,Area,Perimeter\n\n");
    else if(wid && area) auxExport.append("Number of leaf,Width,Area\n\n");
    else if(wid && per) auxExport.append("Number of leaf,Width,Perimeter\n\n");
    else if(len && area && per) auxExport.append("Number of leaf,Length,Area,Perimeter\n\n");
    else if(len && area) auxExport.append("Number of leaf,Length,Area\n\n");
    else if(len && per) auxExport.append("Number of leaf,Length,Perimeter\n\n");
    else if(len) auxExport.append("Number of leaf,Length\n\n");
    else if(wid) auxExport.append("Number of leaf,Width\n\n");
    else if(area && per) auxExport.append("Number of leaf,Area,Perimeter\n\n");
    else if(area) auxExport.append("Number of leaf,Area\n\n");
    else if(per) auxExport.append("Number of leaf,Perimeter\n\n");
}

void MainWindow::on_btnCalc_clicked(){
        if(!database.isOpen()){
            QMessageBox::warning(this, tr("Alert"),tr("Unable to connect to bank, the test will not come to bank."));
        }

        square.clear();
        leavesPCA.clear();
        leaves.clear();
        leavesPer.clear();
        leavesArea.clear();
        remover.clear();
        area = ui->checkBoxArea->isChecked();
        sumareas = ui->checkBoxSumAreas->isChecked();
        wid = ui->checkBoxWid->isChecked();
        len = ui->checkBoxLen->isChecked();
        widlen = ui->checkBoxWidLen->isChecked();
        avedev = ui->checkBoxAveDev->isChecked();
        per = ui->checkBoxPerimeter->isChecked();

        if(wid || len  || area || per){
            if(widlen && !(wid && len)){
                    QMessageBox::warning(this, tr("Alert"),tr("To calculate Width/Length select the Width and Length options as well."));
            }
            else if(sumareas && !area){
                    QMessageBox::warning(this, tr("Alert"),tr("To calculate Sum areas select the Area option as well."));
            }
            else if(ui->endImagem->text().toStdString() == "The image path will come here"){
                QMessageBox::warning(this, tr("Alert"),tr("Select an image."));
            }
            else{
                realAreaSquare = ui->areaQuadrado->text().toFloat();
                spec = ui->esp->text();
                treat = ui->trat->text();
                rep = ui->rep->text();

                image = imread(imagePath, IMREAD_COLOR );

                findObjects();
                surfaceCalc();

                QImage imageaux= QImage((uchar*) image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
                ui->exibirImagem->setPixmap(QPixmap:: fromImage(imageaux).scaled((451*imageaux.width())/imageaux.height(),451));
                ui->labelResult->setVisible(1);
                ui->scrollResult->setVisible(1);
                ui->btnExport->setVisible(1);
                ui->btnRemove->setVisible(1);
                ui->exibirResult->setText(result);
                ui->scrollResult->setWidget(ui->exibirResult);
            }
        }
        else{
             QMessageBox::warning(this, tr("Alert"),tr("Check an option: Area, Width, Lenght or Perimeter."));
        }
}

void MainWindow::on_btnExport_clicked(){
    QFileDialog dialog(this);
    dialog.setViewMode(QFileDialog::Detail);
    QString fileName = QFileDialog::getSaveFileName(this,"");
    if(!fileName.isEmpty()){
        ofstream myfile;
        QString aux = fileName.toStdString().c_str();
        aux = aux.split(".csv")[0];
        aux+= ".csv";
        myfile.open(aux.toStdString());
        myfile.write((auxExport.toStdString().c_str()),auxExport.size());
        myfile.write((auxExport2.toStdString().c_str()),auxExport2.size());
        myfile.close();
    }
}

void MainWindow::on_btnClean_clicked(){
    ui->areaQuadrado->setText("1");
    ui->endImagem->setText("The image path will come here");
    ui->exibirImagem->setText("Select image will come here.");
    ui->labelResult->setVisible(0);
    ui->scrollResult->setVisible(0);
    ui->btnExport->setVisible(0);
    ui->btnRemove->setVisible(0);
    ui->esp->setText("");
    ui->trat->setText("");
    ui->rep->setText("");
}

void MainWindow::on_btnRemove_clicked(){
    bool ok;
    remover = QInputDialog::getText(this, tr("Remove"),tr("Enter the contour number:"),
                                            QLineEdit::Normal,"1,2", &ok);
    if (ok && remover != ""){
        QSqlQuery query;
        if(query.exec("select max(id_Imagem) from imagem")){
             while(query.next()){
                 QString auxIdImagem = query.value(0).toString();
                 query.prepare("delete from imagem where id_Imagem = '"+auxIdImagem+"'");
                 query.exec();
                 query.prepare("delete from folha where id_Imagem = '"+auxIdImagem+"'");
                 query.exec();
             }
        }


        for(int x = 0 ; x < remover.split(",").size(); x++){
            if(!leaves.empty()){
                if((remover.split(",")[x].toInt() - (x+1)) > -1 && (remover.split(",")[x].toInt() - (x+1)) < (int) leaves.size()){
                    leaves.erase(leaves.begin() + (remover.split(",")[x].toInt() - (x+1)));
                    leavesPCA.erase(leavesPCA.begin() + (remover.split(",")[x].toInt() - (x+1)));
                    leavesPer.erase(leavesPer.begin() + (remover.split(",")[x].toInt() - (x+1)));
                    leavesArea.erase(leavesArea.begin() + (remover.split(",")[x].toInt() - (x+1)));
                }else{
                    QMessageBox::warning(this, tr("Alert"),tr("An invalid number has been entered!"));
                }
            }else{
                QMessageBox::warning(this, tr("Alert"),tr("There aren't contours to remove."));
            }
        }


        image.release();
        image = imread(imagePath, IMREAD_COLOR );
        surfaceCalc();

        QImage imageaux= QImage((uchar*) image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        ui->exibirImagem->setPixmap(QPixmap:: fromImage(imageaux).scaled((451*imageaux.width())/imageaux.height(),451));
        ui->exibirResult->setText(result);
        ui->scrollResult->setWidget(ui->exibirResult);

    }
}

void MainWindow::on_tabWidget_currentChanged(){
    QSqlQuery query,query2;
    if(query.exec("select * from imagem order by id_Imagem desc")){
         QString aux = "";
         while(query.next()){
            aux += "\n" + query.value(0).toString() + "\n\n";
            aux += "Image: " + query.value(1).toString();
            aux += "\nSpecies: " + query.value(2).toString();
            aux += "\nTreatment: " + query.value(3).toString();
            aux += "\nReplicate: " + query.value(4).toString();
            aux += "\nScale pattern area: " + query.value(5).toString();
            aux += "\nAverage width: " + query.value(6).toString();
            aux += "\nWidth deviation: " + query.value(7).toString();
            aux += "\nAverage lenght: " + query.value(8).toString();
            aux += "\nLenght deviation: " + query.value(9).toString();
            aux += "\nAverage area: " + query.value(10).toString();
            aux += "\nArea deviation: " + query.value(11).toString();
            aux += "\nSum areas: " + query.value(12).toString();
            aux += "\nAverage perimeter: " + query.value(13).toString();
            aux += "\nPerimeter deviation: " + query.value(14).toString()+ "\n\n";

            QString endImagem = query.value(0).toString();

            if(query2.exec("select * from folha where id_Imagem = '" +endImagem+"'")){
                 while(query2.next()){
                    aux += "Folha: " + query2.value(2).toString() + "\n\n";
                    aux += "\nWidth: " + query2.value(3).toString();
                    aux += "\nLenght: " + query2.value(4).toString();
                    aux += "\nWidth/Length: " + query2.value(5).toString();
                    aux += "\nArea: " + query2.value(6).toString();
                    aux += "\nPerimeter: " + query2.value(7).toString()+ "\n\n";
                }
                 aux+="________________________________________\n";
            }
        }

        if(aux == ""){
            ui->exibirHist->setText("The historic will come here.");
        }else{
            ui->exibirHist->setText(aux);
        }
        ui->scrollHist->setWidget(ui->exibirHist);
        ui->scrollHist->setAlignment(Qt::AlignHCenter);
    }
}

void MainWindow::on_btnClearHistory_clicked(){
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Clear history", "This operation will delete the entire historic. Are you sure?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        query.prepare("delete from folha");
        query.exec();
        query.prepare("delete from imagem");
        query.exec();

        ui->exibirHist->setText("The historic will come here.");
    }
}
