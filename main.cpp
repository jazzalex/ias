#include <QApplication>
#include <ias.h>


int main(int argc, char *argv[]){ 
  QApplication yo(argc,argv);

  ias myIAS;

  return yo.exec();
}

