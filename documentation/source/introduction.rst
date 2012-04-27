Introduction
============

NAViTiA est une suite applicative compl�te permettant la conception, la gestion et la diffusion d'information voyageur multi-modale.

Nous pr�sentons dans cette introduction les 4 niveaux suivants :
* Les fournisseurs d�information.
* Le noyau de communication, qui assure l�interface avec les syst�mes partenaires. Dans notre architecture, le noyau de communication est principalement compos� des services FUSiO (donn�es structurelles) et AlerteTrafic (donn�es �v�nementielles).
* Le SIV qui assure la production des services applicatifs � destination des voyageurs et des partenaires. Dans notre architecture, le SIV est principalement compos� du hub NAViTiA.
* Les canaux d�acc�s � l�information.

Les interfaces de ces sous-syst�mes sont claires et bien document�es. 
* Elles reposent sur des � web services �, ce qui en garantit l�ouverture et la souplesse d�int�gration pour int�grer une probl�matique locale sp�cifique par exemple.
* Elles s�appuient sur la normalisation Transmodel notamment pour ce qui est des interfaces avec les syst�mes des contributeurs ou de l�export du r�f�rentiel consolid� vers d�autres syst�mes.
* Elles sont �volutives. L�approche en service h�berg� facilite l�acc�s � de nouveaux services d�velopp�s dans le cadre d�autres projets. 

.. warning::
La mise � disposition d�interfaces automatis�es avec un nombre toujours plus important d�autres syst�mes est un point fort de notre solution.
::