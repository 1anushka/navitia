/** Exemple de webservice : il affiche le nombre de requ�tes trait�es par le webservice et par le thread courant */

#include "baseworker.h"
#include <iostream>
using namespace webservice;

/** Structure de donn�es globale au webservice
 *  N'est instanci� qu'une seule fois au chargement
 */
struct Data{
  int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, �a ne compile pas
  int count; /// Notre compteur d'appels au webservice
  boost::mutex mut; /// Un mutex pour prot�ger ce cout
  /// Constructeur par d�faut, il est appel� au chargement du webservice
  Data() : nb_threads(8), count(0){
      Configuration * conf = Configuration::get();
      std::cout << conf->strings["application"] <<std::endl;
  }
};

/// Classe associ�e � chaque thread
class Worker : public BaseWorker<Data> {
    int i; /// Compteur de requ�tes sur le thread actuel

    /** Api qui compte le nombre de fois qu'elle a �t� appel�e */
    ResponseData count(RequestData, Data & d) {
        i++;
        ResponseData rd;
        std::stringstream ss;
        ss << "Hello world!!! Ex�cut� par ce thread : " << i << " execut� au total : ";
        d.mut.lock();
        ss << d.count++;
        d.mut.unlock();

        rd.response = ss.str();
        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }



    public:    
    /** Constructeur par d�faut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(Data &) : i(0) {
        register_api("/count",boost::bind(&Worker::count, this, _1, _2), "Api qui compte le nombre d'appels effectu�s");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
