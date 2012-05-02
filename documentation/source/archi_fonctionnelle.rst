Architecture fonctionnelle
==========================

Description des diff�rents modules
**********************************

.. aafig::
  :fill:#FF6600

/------------+ /-----+ /------------+     /------------+ /-----+ /------------+
|Donn�es     | |     | |Donn�es     |     |Donn�es     | |     | |Donn�es     |
|Transporteur| | ... | |Transporteur|     |Transporteur| | ... | |Transporteur|
|1           | |     | |n           |     |1           | |     | |n           |
+------------+ +-----+ +------------+     +------------+ +-----+ +------------+
    

.. aafig::

               /-----\                                /------------\
               |FUSiO|                                |AlerteTrafic|
               \-----/                                \------------/
                  |              +--------------+            |
                  |             /Espace de mise  \           |
                  \----------->/� disposition des \<---------/
                              /donn�es th�orique et\
                             /     temps r�el       \
                            +------------------------+
                                         |
                                        /|\
                                       / | \
                                      v  v  v
.. aafig::
  :fill:#4F81BD
                                /-----------------\
                                |Moteurs de calcul|\
                                \-----------------/|
                                 \-----------------/
                                        /|\
                                       / | \
                                       | | |
                                       \ | /
                                        \|/
                                         v
                                    /---------\
                                    | NAViTiA |\
                                    \---------/|
                                     \---------/

