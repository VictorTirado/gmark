MATCH (x0)-[:p1*]->(x1), (x1)-[:p1|p1*]->(x2), (x2)-[:p1*]->(x3), (x3)<-[:p0]-()-[:p0]->()-[:p1]->(x4) RETURN DISTINCT x0;
