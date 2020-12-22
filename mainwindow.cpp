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

using namespace cv;
using namespace std;

Mat thresh,image;

String endImagem;

float areaQuadrado;
bool area,sumareas,wid,len,widlen,avedev,rem,per;

RNG rng(12345);

vector<vector<Point> > square;
vector<vector<Point> > squareApprox;
vector<vector<Point> > leaves;
vector<vector<Point> > leavesPCA;
vector<vector<Point> > leavesPCAapprox;

QString result = "",auxExport = "",auxExport2 = "";
QString remover,esp,trat,rep;

static QSqlDatabase banco;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
        ui->setupUi(this);

        QString dir = qApp->applicationDirPath();
        QString dirbanco = dir+"/bd/bd.db";

        QString dirdriver = dir+"/sqldrivers/";
        QCoreApplication::addLibraryPath(dirdriver);

        banco = QSqlDatabase::addDatabase("QSQLITE");
        banco.setDatabaseName(dirbanco);
        if(!banco.open()){
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
        endImagem = ui->endImagem->text().toStdString();
        image = imread( endImagem, IMREAD_COLOR );
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

Point GetPointAfterRotate(Point inputpoint,Point center,double angle){
    Point preturn;
    preturn.x = (inputpoint.x - center.x)*cos(-1*angle) - (inputpoint.y - center.y)*sin(-1*angle)+center.x;
    preturn.y = (inputpoint.x - center.x)*sin(-1*angle) + (inputpoint.y - center.y)*cos(-1*angle)+center.y;
    return preturn;
}

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

static double angle( Point pt1, Point pt2, Point pt0 ){
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

void pca(vector<vector<Point> >& contours, int i){
    Point2f* pos = new Point2f();
    double dOrient =  getOrientation(contours[i], *pos);
    int xmin = 99999;
    int xmax = 0;
    int ymin = 99999;
    int ymax = 0;

    for (size_t j = 0;j<contours[i].size();j++){
        contours[i][j] = GetPointAfterRotate(contours[i][j],(Point)*pos,dOrient);
        if (contours[i][j].x < xmin)
            xmin = contours[i][j].x;
        if (contours[i][j].x > xmax)
            xmax = contours[i][j].x;
        if (contours[i][j].y < ymin)
            ymin = contours[i][j].y;
        if (contours[i][j].y > ymax)
            ymax = contours[i][j].y;
     }
}

static void findObjects(){
    Mat gray;

    vector<vector<Point> > contours;

    vector<Point> approx;

    cvtColor( image, gray, COLOR_BGR2GRAY );

    threshold( gray, thresh, 0, 255, THRESH_BINARY_INV|THRESH_OTSU);

    findContours(thresh, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    for(size_t i = 0; i < contours.size(); i++){
        approxPolyDP(contours[i], approx, arcLength(contours[i], true)*0.02, true);

        //fabs(contourArea(approx)) > 10000 &&
        if(approx.size() == 4 && fabs(contourArea(approx)) > 1000 && fabs(contourArea(approx)) < 999999999 && isContourConvex(approx) ){
            double maxCosine = 0;

            for( int j = 2; j < 5; j++ ){
                double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                maxCosine = MAX(maxCosine, cosine);
             }

             if( maxCosine < 0.3 ){
                 squareApprox.push_back(approx);
                 square.push_back(contours[i]);
             }
             else{
                 leaves.push_back(contours[i]);
                 pca(contours,i);
                 leavesPCA.push_back(contours[i]);

             }
        }else if(fabs(contourArea(approx)) > 1000 && fabs(contourArea(approx)) < 999999999){
            leaves.push_back(contours[i]);
            pca(contours,i);
            leavesPCA.push_back(contours[i]);

        }
    }
}

static void surfaceCalc(){
    result.clear();
    auxExport.clear();
    auxExport2.clear();

    //---------------------Variaveis auxiliares calculos-----------------------

    float largSquare,compSquare,sum=0.0;
    float mL=0.0,mC=0.0,mA=0.0,mP=0.0;
    float dL=0.0,dC=0.0,dA=0.0,dP=0.0;
    float L[leaves.size()],C[leaves.size()],A[leaves.size()],P[leaves.size()], LC[leaves.size()];

    //---------------------Variaveis auxiliares banco-----------------------

    QSqlQuery query;
    QString nome = QString::fromStdString(endImagem);
    nome = nome.split("/")[nome.split("/").size()-1];
    char dateStr [9]; _strdate(dateStr); char timeStr [9]; _strtime(timeStr);
    QString id_Imagem = dateStr; id_Imagem+=" "; id_Imagem+= timeStr;

    //-------------------SQUARE----------------------

    //Calculos larg e comp quadrado em pixels

    if(wid || len){

        /*
        //---------------Quadrado com vertices------------

        largSquare = sqrt((pow((squareApprox[0][2].x - squareApprox[0][1].x),2)+pow((squareApprox[0][2].y - squareApprox[0][1].y),2)));
        compSquare = sqrt((pow((squareApprox[0][1].x - squareApprox[0][0].x),2)+pow((squareApprox[0][1].y - squareApprox[0][0].y),2)));
        */


        /*
         //---------------Quadrado com boundingRect------------

        vector<Rect> boundRect(square.size() );
        boundRect[0] = boundingRect(square[0]);
        largSquare = sqrt((pow((boundRect[0].tl().x - boundRect[0].tl().x),2)+pow((boundRect[0].br().y - boundRect[0].tl().y),2)));
        compSquare = sqrt((pow((boundRect[0].tl().x  - boundRect[0].br().x),2)+pow((boundRect[0].tl().y - boundRect[0].tl().y),2)));
        */


        /*
        //---------------Quadrado com minAreaRect------------

        vector<RotatedRect> minRect(square.size());
        minRect[0] = minAreaRect(square[0]);
        Point2f rect_points[4];
        minRect[0].points( rect_points );
        largSquare = sqrt((pow((rect_points[1].x - rect_points[2].x),2)+pow((rect_points[1].y - rect_points[2].y),2)));
        compSquare = sqrt((pow((rect_points[3].x - rect_points[2].x),2)+pow((rect_points[3].y - rect_points[2].y),2)));
        */


        /*
        //---------------Quadrado aproximado com vertices (média) ------------

        float vertA = sqrt((pow((squareApprox[0][1].x - squareApprox[0][0].x),2)+pow((squareApprox[0][1].y - squareApprox[0][0].y),2)));
        float vertB = sqrt((pow((squareApprox[0][2].x - squareApprox[0][1].x),2)+pow((squareApprox[0][2].y - squareApprox[0][1].y),2)));
        float vertC = sqrt((pow((squareApprox[0][3].x - squareApprox[0][2].x),2)+pow((squareApprox[0][3].y - squareApprox[0][2].y),2)));
        float vertD = sqrt((pow((squareApprox[0][3].x - squareApprox[0][0].x),2)+pow((squareApprox[0][3].y - squareApprox[0][0].y),2)));

        largSquare = compSquare = (vertA + vertB + vertC + vertD)/4;
        */

        /*
        //---------------Quadrado original com vertices adjacentes minArea (média) ------------

        vector<RotatedRect> minRect(square.size());
        minRect[0] = minAreaRect(square[0]);
        Point2f rect_points[4];
        minRect[0].points( rect_points );

        float vertA = sqrt((pow((rect_points[1].x - rect_points[0].x),2)+pow((rect_points[1].y - rect_points[0].y),2)));
        float vertB = sqrt((pow((rect_points[2].x - rect_points[1].x),2)+pow((rect_points[2].y - rect_points[1].y),2)));
        float vertC = sqrt((pow((rect_points[3].x - rect_points[2].x),2)+pow((rect_points[3].y - rect_points[2].y),2)));
        float vertD = sqrt((pow((rect_points[3].x - rect_points[0].x),2)+pow((rect_points[3].y - rect_points[0].y),2)));

        largSquare = compSquare = (vertA + vertB + vertC + vertD)/4;
        */

        /*cout<<"Distancia 0 e 1: "<<vertA;
        cout<<"\nDistancia 1 e 2: "<<vertB;
        cout<<"\nDistancia 2 e 3: "<<vertC;
        cout<<"\nDistancia 3 e 0: "<<vertD;
        */

        //---------------Quadrado com arcLength ------------

        largSquare = compSquare = (arcLength(square[0],true))/4;

    }

    //Desenhar quadrado

    const Point* p = &square[0][0];
    int n = (int)square[0].size();
    polylines(image, &p, &n, 1, true, Scalar(0,255,0), 10, LINE_AA);

    //---------------------LEAFS-----------------------

    //Remover folhas informadas pelo usuario

    if(rem){
        for(size_t x = 0 ; x < remover.split(",").size(); x++){
            leaves.erase(leaves.begin() + (remover.split(",")[x]).toInt() - (x+1));
            leavesPCA.erase(leavesPCA.begin() + (remover.split(",")[x]).toInt() - (x+1));
        }
    }

    //-------------Calculo das dimensoes----------------

    vector<Rect> boundRect(leavesPCA.size() );

    for(size_t i = 0; i < leavesPCA.size(); i++ ){

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

            float aux = sqrt((pow((boundRect[i].tl().x - boundRect[i].tl().x),2)+pow((boundRect[i].br().y - boundRect[i].tl().y),2)));
            aux = (aux * sqrt(float(areaQuadrado)))/largSquare;

            float aux2 = sqrt((pow((boundRect[i].tl().x  - boundRect[i].br().x),2)+pow((boundRect[i].tl().y - boundRect[i].tl().y),2)));
            aux2 = (aux2 * sqrt(float(areaQuadrado)))/compSquare;

            if(aux2 > aux){
                 if(avedev && wid) mL += aux;
                 if(avedev && len) mC += aux2;
                 if(wid){
                     result.append("\nWidth: "); result.append(QString::number(aux));
                     L[i] = aux;
                 }
                 if(len){
                     result.append("\nLength: "); result.append(QString::number(aux2));
                     C[i] = aux2;
                 }

                 if(widlen){
                     result.append("\nWidth/Length: "); result.append(QString::number(aux/aux2));
                     LC[i] = aux/aux2;
                 }

            }else{

                 if(avedev && wid) mL += aux2;
                 if(avedev && len) mC += aux;
                 if(wid){
                     result.append("\nWidth: "); result.append(QString::number(aux2));
                     L[i] = aux2;
                 }
                 if(len){
                     result.append("\nLength: "); result.append(QString::number(aux));
                     C[i] = aux;
                 }
                 if(widlen){
                     result.append("\nWidth/Length: "); result.append(QString::number(aux2/aux));
                     LC[i] = aux2/aux;
                 }
             }
        }

        //_____________Calculo Area_____________

        if(area){
            float auxArea = ((contourArea(leavesPCA[i]) * areaQuadrado)/ contourArea(square[0]));

            //float auxArea = ((contourArea(leavesPCA[i]) * areaQuadrado)/contourArea(squareApprox[0]));

            if(sumareas) sum += auxArea;

            result.append("\nArea: "); result.append(QString::number(auxArea));

            if(avedev && area) mA += auxArea;

            A[i] = auxArea;
        }

        //_____________Calculo Perimetro_____________


        if(per){
            float perSquare = sqrt(float(areaQuadrado))* 4;

            float auxPer = ((arcLength(leavesPCA[i],true) * perSquare)/arcLength(square[0],true));

            //float auxPer = ((arcLength(leavesPCA[i],true) * perSquare)/arcLength(squareApprox[0],true));

            result.append("\nPerimeter: "); result.append(QString::number(auxPer));

            if(avedev && per) mP += auxPer;

            P[i] = auxPer;
        }

        result.append("\n\n");

        //_____________Export_____________

        if(wid && len && widlen && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(LC[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(wid && len && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(LC[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(LC[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(wid && len && widlen){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(LC[i])); auxExport2.append("\n");
        }
        else if(wid && len && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append("\n");
        }
        else if(wid && len && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(wid && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(wid && len){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append("\n");
        }
        else if(wid && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append("\n");
        }
        else if(wid && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(len && area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(len && area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append("\n");
        }
        else if(len && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(len){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(C[i])); auxExport2.append("\n");
        }
        else if(wid){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(L[i])); auxExport2.append("\n");
        }
        else if(area && per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
        }
        else if(area){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(A[i])); auxExport2.append("\n");
        }
        else if(per){
            auxExport2.append(QString::number(i+1));  auxExport2.append(",");
            auxExport2.append(QString::number(P[i])); auxExport2.append("\n");
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
            mL = mL / leavesPCA.size();
            result.append("\nAverage width: "); result.append(QString::number(mL));
        }

        if(avedev && len){
            mC = mC / leavesPCA.size();
            result.append("\nAverage lenght: "); result.append(QString::number(mC));
        }

        if(avedev && area){
            mA = mA / leavesPCA.size();
            result.append("\nAverage area: "); result.append(QString::number(mA));
        }

        if(avedev && per){
            mP = mP / leavesPCA.size();
            result.append("\nAverage perimeter: "); result.append(QString::number(mP));
        }

         result.append("\n\n");

        //_____________Desvio_____________

        if(avedev && wid){
            for(size_t i = 0; i < leavesPCA.size(); i++ ){
                dL += pow(L[i]-mL,2);
            }
            dL = sqrt(dL / leavesPCA.size());
            result.append("\nWidth deviation: "); result.append(QString::number(dL));
        }

        if(avedev && len){
            for(size_t i = 0; i < leavesPCA.size(); i++ ){
                dC += pow(C[i]-mC,2);
            }
            dC = sqrt(dC / leavesPCA.size());
            result.append("\nLenght deviation: "); result.append(QString::number(dC));
        }

        if(avedev && area){
            for(size_t i = 0; i < leavesPCA.size(); i++ ){
                dA += pow(A[i]-mA,2);
            }
            dA = sqrt(dA / leavesPCA.size());
            result.append("\nArea deviation: "); result.append(QString::number(dA));
        }

        if(avedev && per){
            for(size_t i = 0; i < leavesPCA.size(); i++ ){
                dP += pow(P[i]-mP,2);
            }
            dP = sqrt(dP / leavesPCA.size());
            result.append("\nPerimeter deviation: "); result.append(QString::number(dP));
        }

         result.append("\n\n");
     }

    //_____________Inserindo Media e Desvio no banco e no export_____________


    if(avedev && area && wid && len && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+
                      QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");

        auxExport2.append("Sum:,,,");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && len && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+
                      QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");

        auxExport2.append("Sum:,,,");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && len && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && len && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");

        auxExport2.append("Sum:,,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && area && wid && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)
                      +","+QString::number(mA)+","+QString::number(dA)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");
    }
    else if(avedev && wid && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        if(widlen) auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && wid && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "comp_Medio,comp_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mC)+","+QString::number(dC)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mC));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dC));auxExport2.append("\n");
    }
    else if(avedev && area && sumareas && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio,sumareas,"
                      "per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mA)
                      +","+QString::number(dA)+","+QString::number(sum)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");

        auxExport2.append("Sum:,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mA)
                      +","+QString::number(dA)+","+QString::number(sum)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");

        auxExport2.append("Sum:,"); auxExport2.append(QString::number(sum));
    }
    else if(avedev && area && wid && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio,per_Media,per_desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mA)+","+QString::number(dA)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && area && wid){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio,"
                      "area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+","+QString::number(mA)+","+QString::number(dA)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");
    }
    else if(avedev && area && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio,per_Media,per_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mC)
                      +","+QString::number(dC)+","+QString::number(mA)+","+QString::number(dA)+","
                      +QString::number(mP)+","+QString::number(dP)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && area && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,"
                      "comp_Medio,comp_Desvio,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mC)
                      +","+QString::number(dC)+","+QString::number(mA)+","+QString::number(dA)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");
    }
    else if(avedev && area && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,area_Media,area_Desvio,per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+
                      QString::number(areaQuadrado)+","+QString::number(mA)+","+
                      QString::number(dA)+","+QString::number(mP)+","+QString::number(dP)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mA));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dA)); auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && area){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,area_Media,area_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mA)+","+QString::number(dA)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mA));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dA)); auxExport2.append("\n");
    }
    else if(avedev && wid && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,larg_Media,larg_Desvio, per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+
                      QString::number(areaQuadrado)+","+QString::number(mL)+","+
                      QString::number(dL)+","+QString::number(mP)+","+QString::number(dP)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && wid){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,larg_Media,larg_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mL)
                      +","+QString::number(dL)+")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mL));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dL));auxExport2.append("\n");
    }
    else if(avedev && len && per){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,"
                      "area_Quad,comp_Medio,comp_Desvio, per_Media,per_Desvio) values "
                      "('"+id_Imagem+"','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+
                      QString::number(areaQuadrado)+","+QString::number(mC)+","+
                      QString::number(dC)+","+QString::number(mP)+","+QString::number(dP)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append(",");
        auxExport2.append(QString::number(mP));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append(",");
        auxExport2.append(QString::number(dP));auxExport2.append("\n");
    }
    else if(avedev && len){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,comp_Medio,comp_Desvio) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(mC)+","+QString::number(dC)
                      +")");
        query.exec();

        auxExport2.append("Average:,");
        auxExport2.append(QString::number(mC));auxExport2.append("\n");

        auxExport2.append("Deviation:,");
        auxExport2.append(QString::number(dC));auxExport2.append("\n");
    }
    else if(area && sumareas){
        query.prepare("insert into imagem(id_Imagem,nome,especie,tratamento,repeticao,area_Quad,sumareas) values ('"+id_Imagem+
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+","+QString::number(sum)+")");
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
                      "','"+nome+"','"+esp+"','"+trat+"','"+rep+"',"+QString::number(areaQuadrado)+")");
        query.exec();
    }

    //_____________Inserindo Area,Largura,Comprimento e Perimetro no banco_____________

    for(size_t i = 0; i < leavesPCA.size(); i++ ){
        if(wid && len && area && widlen && per){
                    query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,"
                                  "largcomp,per) values ("+QString::number((i+1))+",'"+
                                  id_Imagem+"',"+QString::number(A[i])+","+
                                  QString::number(L[i])+","+QString::number(C[i])+","+
                                  QString::number(LC[i])+","+QString::number(P[i])+")");
                    query.exec();
                }
        else if(wid && len && area && widlen){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,largcomp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(L[i])+","+QString::number(C[i])+","+QString::number(LC[i])+")");
            query.exec();
        }
        else if(wid && len && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(L[i])+","+QString::number(C[i])+","+
                          QString::number(P[i])+")");
            query.exec();
        }
        else if(wid && len && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(L[i])+","+QString::number(C[i])+")");
            query.exec();
        }
        else if(wid && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(L[i])+","+QString::number(P[i])+")");
            query.exec();
        }
        else if(wid && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,larg) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(L[i])+")");
            query.exec();
        }
        else if(len && area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(C[i])+","+QString::number(P[i])+")");
            query.exec();
        }
        else if(len && area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(C[i])+")");
            query.exec();
        }
        else if(area && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,area,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +","+QString::number(P[i])+")");
            query.exec();
        }
        else if(area){
            query.prepare("insert into folha(num_Folha,id_Imagem,area) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(A[i])
                          +")");
            query.exec();
        }
        else if(wid && len && widlen && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,largcomp,per)"
                          " values ("+QString::number((i+1))+",'"+id_Imagem+"',"+
                          QString::number(L[i])+","+QString::number(C[i])+","+
                          QString::number(LC[i])+","+QString::number(P[i])+")");
            query.exec();
        }
        else if(wid && len && widlen){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,largcomp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(L[i])+","
                          +QString::number(C[i])+","+QString::number(LC[i])+")");
            query.exec();
        }
        else if(wid && len && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(L[i])+","
                          +QString::number(C[i])+","+QString::number(P[i])+")");
            query.exec();
        }
        else if(wid && len){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(L[i])+","
                          +QString::number(C[i])+")");
            query.exec();
        }
        else if(wid && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+
                          QString::number(L[i])+","+QString::number(P[i])+")");
            query.exec();
        }
        else if(wid){
            query.prepare("insert into folha(num_Folha,id_Imagem,larg) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(L[i])+")");
            query.exec();
        }
        else if(len && per){
            query.prepare("insert into folha(num_Folha,id_Imagem,comp,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(C[i])
                          +","+QString::number(P[i])+")");
            query.exec();
        }
        else if(len){
            query.prepare("insert into folha(num_Folha,id_Imagem,comp) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(C[i])+")");
            query.exec();
        }
        else if(per){
            query.prepare("insert into folha(num_Folha,id_Imagem,per) values ("
                          +QString::number((i+1))+",'"+id_Imagem+"',"+QString::number(P[i])+")");
            query.exec();
        }
    }

    //_____________Export_____________

    auxExport.append("Image:," + nome);
    auxExport.append("\nSpecies:," + esp);
    auxExport.append("\nTreatment:," + trat);
    auxExport.append("\nReplicate:," + rep);
    auxExport.append("\nScale pattern area:,"); auxExport.append(QString::number(areaQuadrado));
    auxExport.append("\nNumber of leaves:,"); auxExport.append(QString::number(leavesPCA.size()));
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
        if(!banco.isOpen()){
            QMessageBox::warning(this, tr("Alert"),tr("Unable to connect to bank, the test will not come to bank."));
        }

        squareApprox.clear();
        square.clear();
        leavesPCA.clear();
        leaves.clear();
        remover.clear();
        rem = false;
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

                auto movie = new QMovie(this);
                movie->setFileName("./icons/loading.gif");
                connect(movie, &QMovie::frameChanged, [=]{
                    ui->exibirImagem->setPixmap((movie->currentPixmap()));
                });
                movie->start();

                areaQuadrado = ui->areaQuadrado->text().toFloat();
                esp = ui->esp->text();
                trat = ui->trat->text();
                rep = ui->rep->text();


               //__________________________Sombras_________________________
                /*namedWindow("rgb",WINDOW_NORMAL);
                imshow("rgb",image);

                Mat HSV_img, MASKED_img,res;

                cvtColor(image, HSV_img, COLOR_BGR2HSV);

                inRange(HSV_img, Scalar(0,0,90), Scalar(180,255,255), MASKED_img);
                HSV_img.setTo(Scalar(0,0,255),MASKED_img);

                //bitwise_and(image,image, res, MASKED_img);

                image.release();
                cvtColor(HSV_img,image,COLOR_HSV2BGR);

                namedWindow("mask",WINDOW_NORMAL);
                imshow("mask",MASKED_img);



              Mat imageHSV;
                cvtColor(image, imageHSV,COLOR_BGR2HSV);
                vector<Mat> channels;
                split(imageHSV, channels );
                Mat H = channels[0];

                for(int i = H.rows-1; i < (H.rows); i++){
                        for(int j = 0; j < (H.cols); j++){
                             if(H.at<uchar>(i, j) > 108){
                                channels[0].at<uchar>(i, j) = 0;
                                channels[1].at<uchar>(i, j) = 0;
                                channels[2].at<uchar>(i, j) = 255;
                            }
                        }
                    }

                merge(channels,imageHSV);
                cvtColor(imageHSV,image,COLOR_HSV2BGR);
                namedWindow("result",WINDOW_NORMAL);
                imshow("result",image);


                //___________________________________________________________*/

                image = imread( endImagem, IMREAD_COLOR );

                findObjects();
                surfaceCalc();

                movie->stop();
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
    ui->areaQuadrado->setText("9");
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
    rem = true;
    bool ok;
    remover = QInputDialog::getText(this, tr("Remove"),tr("Contours to remove:"),
                                            QLineEdit::Normal,"Example: 1,2 (Ascending order)", &ok);
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
        image.release();
        image = imread( endImagem, IMREAD_COLOR );
        surfaceCalc();

        QImage imageaux= QImage((uchar*) image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        ui->exibirImagem->setPixmap(QPixmap:: fromImage(imageaux).scaled((451*imageaux.width())/imageaux.height(),451));
        ui->exibirResult->setText(result);
        ui->scrollResult->setWidget(ui->exibirResult);

    }
}

void MainWindow::on_tabWidget_currentChanged(int index){
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

