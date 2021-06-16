/*
static void draw( Mat& image, const vector<vector<Point> >& contornos){
    for( size_t i = 0; i < contornos.size(); i++ ){
        const Point* p = &contornos[i][0];
        int n = (int)contornos[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,0,0), 20, LINE_AA);
    }

    //namedWindow("Window 3", WINDOW_NORMAL);

    //imshow("Window 3",image);

    imwrite("contornos.jpg", image);
}


static void draw2( Mat& image, const vector<vector<Point> >& squares,const vector<vector<Point> >& others){
    for( size_t i = 0; i < squares.size(); i++ ){
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,0,0), 20, LINE_AA);
    }
    for( size_t i = 0; i < others.size(); i++ ){
        const Point* p = &others[i][0];
        int n = (int)others[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,0,0), 20, LINE_AA);
    }

    //namedWindow("Window 4", WINDOW_NORMAL);

    //imshow("Window 4",image);

    imwrite("filtragem.jpg", image);

    //imwrite("pca.jpg", image);
}


static void drawSquares( Mat& image, const vector<vector<Point> >& squares,const vector<vector<Point> >& others){
    for( size_t i = 0; i < squares.size(); i++ ){
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,255,0), 20, LINE_AA);
    }
    for( size_t i = 0; i < others.size(); i++ ){
        const Point* p = &others[i][0];
        int n = (int)others[i].size();
        polylines(image, &p, &n, 1, true, Scalar(255,0,0), 20, LINE_AA);
    }

    //namedWindow("Window 5", WINDOW_NORMAL);

    imshow("Window 5",image);

    imwrite("deteccao.jpg", image);
}
*/
