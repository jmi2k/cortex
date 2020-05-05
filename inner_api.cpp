
#include "inner_api.h"

//InnerAPI::InnerAPI(std::shared_ptr<CRDT::CRDTGraph> _G)
InnerAPI::InnerAPI(CRDT::CRDTGraph *G_)
{
    G = G_;
}

/// Computation of resultant RTMat going from A to common ancestor and from common ancestor to B (inverted)
InnerAPI::ListsPtr InnerAPI::setLists(const std::string &destId, const std::string &origId)
{
    std::list<EdgeAttribs> listA, listB;

    auto a = G->getNode(origId);
    auto b = G->getNode(destId);

	if (a->id() == -1)
		throw CRDT::DSRException("Cannot find node: \"" + origId + "\"");
	if (b->id() == -1)
		throw CRDT::DSRException("Cannot find node: "+ destId +"\"");

	int minLevel = std::min(a->getLevel(), b->getLevel());
	while (a->getLevel() >= minLevel)
	{
        //qDebug() << "listaA" << a->id() << a->getLevel() << a->getParentId();
		auto p_node = G->getNode(a->getParentId());
      	if(p_node->id() == -1)
			break;
        listA.push_back(p_node->edge(a->id(), "RT"));   // the downwards RT link from parent to a
		a = p_node;
	}
	while (b->getLevel() >= minLevel)
	{
        //qDebug() << "listaB" << b->id() << b->getLevel();
		auto p_node = G->getNode(b->getParentId());
		if(p_node->id() == -1)
			break;
        listB.push_front(p_node->edge(b->id(), "RT"));
		b = p_node;
	}
	// while (b->id() != a->id())  		// Estaba en InnerModel pero no sé bien cuándo hace falta
	// {
	// 	listA.push_back(a);
	// 	listB.push_front(b);
	// 	a = G->getNode(a->getParentId());
	// 	b = G->getNode(b->getParentId());
	// }
    return std::make_tuple(listA, listB);
}

RTMat InnerAPI::getTransformationMatrixS(const std::string &dest, const std::string &orig)
{
	RTMat ret;
	// if (localHashTr.contains(QPair<QString, QString>(to, from)))
	// {
	// 	ret = localHashTr[QPair<QString, QString>(to, from)];
	// }
	// else
	// {

    auto [listA, listB] = setLists(dest, orig);
    // for(auto a : listA)	
    //     qDebug() << QString::fromStdString(a.label());
    // for(auto b : listB)
    //     qDebug() << QString::fromStdString(b.label());
    auto rt = [](EdgeAttribs &edge){ auto &ats = edge.attrs(); return RTMat(  std::stod(ats["rx"].value()),
                                                        					  std::stod(ats["ry"].value()),
                                                        					  std::stod(ats["rz"].value()),
                                                        					  std::stod(ats["tx"].value()),
                                                        					  std::stod(ats["ty"].value()),
                                                        					  std::stod(ats["tz"].value()));}; 
    for(auto &edge: listA )
    {
        ret = ((RTMat)(rt(edge))).operator*(ret);
        //rt(edge).print("ListA");
    }
    for(auto &edge: listB )
    {
        ret = rt(edge).invert() * ret;
        //rt(edge).print("ListB");
    }
    //	localHashTr[QPair<QString, QString>(to, from)] = ret;
    //}
	return ret;
}

QVec InnerAPI::transformS(const std::string &destId, const QVec &initVec, const std::string &origId)
{
	if (initVec.size()==3)
	{
		return (getTransformationMatrixS(destId, origId) * initVec.toHomogeneousCoordinates()).fromHomogeneousCoordinates();
	}
	else if (initVec.size()==6)
	{
		const QMat M = getTransformationMatrixS(destId, origId);
		const QVec a = (M * initVec.subVector(0,2).toHomogeneousCoordinates()).fromHomogeneousCoordinates();
		const Rot3D R(initVec(3), initVec(4), initVec(5));

		const QVec b = (M.getSubmatrix(0,2,0,2)*R).extractAnglesR_min();
		QVec ret(6);
		ret(0) = a(0);
		ret(1) = a(1);
		ret(2) = a(2);
		ret(3) = b(0);
		ret(4) = b(1);
		ret(5) = b(2);
		return ret;
	}
	else
	{
		throw CRDT::DSRException("InnerModel::transform was called with an unsupported vector size.");
	}
};

 QVec InnerAPI::transformS( const std::string &destId, const std::string &origId)
 {
	return transformS(destId, QVec::vec3(0.,0.,0.), origId);
 }

 QVec InnerAPI::transform(const QString & destId, const QVec &origVec, const QString & origId)
 {
	return transformS(destId.toStdString(), origVec, origId.toStdString());
 }

 QVec InnerAPI::transform( const QString &destId, const QString & origId)
 {
 	return transformS(destId.toStdString(), QVec::vec3(0.,0.,0.), origId.toStdString());
 }

