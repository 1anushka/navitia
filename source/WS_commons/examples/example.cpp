/** Exemple de webservice : il affiche le nombre de requ�tes trait�es par le webservice et par le thread courant */

#include "baseworker.h"
typedef BaseWorker<int> bw;

using namespace webservice;
using namespace boost::posix_time;

/** Structure de donn�es globale au webservice
 *  N'est instanci� qu'une seule fois au chargement
 */
struct Data{
  int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, �a ne compile pas
  int count; /// Notre compteur d'appels au webservice
  boost::mutex mut; /// Un mutex pour prot�ger ce cout
  /// Constructeur par d�faut, il est appel� au chargement du webservice
  Data() : nb_threads(8), count(0){
  }
};

/// Classe associ�e � chaque thread
class Worker{
    int i; /// Compteur de requ�tes sur le thread actuel
    public:
    Worker() : i(0) {} /// Constructeur par d�faut
    
    /// Fonction appel�e � chaque requ�te. Il faut respecter cette signature !
    ResponseData operator()(const RequestData &, Data & d){
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
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
