MATCH (x0)<-[:p3]-()<-[:p0]-()-[:p0]->()<-[:p0]-(x1), (x0)<-[:p3]-()-[:p1]->()-[:p2]->()<-[:p2]-(x2), (x1)<-[:p3]-()-[:p3]->()<-[:p3]-()-[:p1]->(x3) RETURN "true" LIMIT 1;
