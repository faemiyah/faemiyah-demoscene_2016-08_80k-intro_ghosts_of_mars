function result = GenerateControlPoint(prevpoint, point, nextpoint, givenext)
	% Lasken kontrollipisteet nyt niin ett� viivan tangentti on suora
	% pistett� edellisest� interpolaatiopisteest� sit� seuraavaan
	result = nextpoint - prevpoint;
	result = result/norm(result);

	% katotaan suunta, - edelliseen, + seuraavaan
	% painotetaan kanssa kontrollipisteen suuruutta
	% polun pituuden neli�juurella, mutta t�m� on	vaan randomi valinta, voi olla mit� vaan
	if(givenext==true)
		dist = (nextpoint-point);
		dist = sqrt(norm(dist));
		result = result*dist;
	else
		dist = (point-prevpoint);
		dist = sqrt(norm(dist));
		result = -result*dist;
	end
end