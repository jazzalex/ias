#include <QApplication>
#include <ias.h>


/// MAIN FUNCTION AS THE STARTING POINT OF THE APPLICATION
int main(int argc, char *argv[]){ 
  QApplication yo(argc,argv);

  ias myIAS;

  return yo.exec();
}

