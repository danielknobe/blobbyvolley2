g= -CONST_BALL_GRAVITY
pg= -CONST_BLOBBY_GRAVITY
v0=14.5
v_p=4.5
pj= pg/2
r1= CONST_BALL_RADIUS 
h=31.5+19+25
estt=0
p=0.654
s=math.random()
function reset()

	est=0
	imp,imp1,imp2=0,0,0
	nettime=0
	touch=0
	esto1=0
	esto2=0
	esto3=0
	esto4=0
	esto5=0
	esto6=0
	phase=0
	valid=1
	active=1
	nettime=0
	y1p=posy()
end
	
function yb(y,vy,t)
	return y+(vy-g/10)*t-1/2*g*t^2
end

function move(x)
	if (posx()<x) and (math.abs(posx()-x)>2.26) then
			right()
		elseif (posx()>x) and (math.abs(posx()-x)>2.26) then
			left()
	end
end

function tb(y,vy,height,typ)
	local sgn=0
	if (typ==1)
		then
			sgn=-1
		else
			sgn=1
	end
	if vy^2/g^2-vy/(5*g)+1/100+2*(y-height)/g<0 then
		return -1
	else
		return -1/10+vy/g+sgn*math.sqrt( vy^2/g^2-vy/(5*g)+1/100+2*(y-height)/g )
	end
end

function time(t)
	return 1/5*math.ceil(5*t)
end

function pos2(x)

	local x1,x2
	x1=4.5*math.ceil((1/4.5)*x)
	x2=4.5*math.floor((1/4.5)*x)
	if (x1<0) or (x2<0) then
			return 0
		else
			if (math.abs(x1-x)<math.abs(x2-x)) then
					return x1
				else
					return x2
			end
	end
end

function pos(x)
	return pos2(x)+(posx()-pos2(posx()))
end

function yp(t)
	return 144.5+(14.5+pj/2+pg/10)*t-1/2*(pg-pj)*t^2
end

function tp(y)
	return (14.5+pj/2+pg/10)/(pg-pj)-math.sqrt(((14.5+pj/2+pg/10)/(pg-pj))^2-(y-144.5)/(1/2*(pg-pj)))
end

function tp2(y1p,v1p)
	if (y1p>146) then
	return (v1p/0.88-1/2+math.sqrt((v1p/0.88-1/2)^2-(144.5-y1p)/0.44))
	else
	return 0
	end
end

function time(t)
	return 1/5*math.ceil(5*t)
end


-- </Abschnitt 2>

function collide(x,y,vx,vy,objx,objy,r2)

	local t1=time(math.max(tb(y,vy,objy-(r1+r2),1)-0.2,0))
	local t2=time(tb(y,vy,objy+(r1+r2),1)+0.2)
	local t3=time(math.max(tb(y,vy,objy+(r1+r2),2)-0.2,0))
	local t4=time(tb(y,vy,objy-(r1+r2),2)+0.2)
	local t=-1
	if (t1<t2)
		then
			for testimate=t1,t2,0.2 do
				if (yb(y,vy,testimate)<=316) and (x+vx*testimate>=400-(31.5+7)) then
					return -1
				end
				if ( (yb(y,vy,testimate)-objy)^2+(x+vx*testimate-objx)^2 < (r1+r2)^2)
					then
						t=testimate
						break
				end
			end
	end

	if (t<=0) and (t3<t4)
		then
			for testimate=t3,t4,0.2 do
				if (yb(y,vy,testimate)<=316) and (x+vx*testimate>=400-(31.5+7)) then
					return -1
				end
				if ( (yb(y,vy,testimate)-objy)^2+(x+vx*testimate-objx)^2 < (r1+r2)^2)
					then
						t=testimate
						break
				end
			end
	end

	if (t<=0) or (t>150) then t=-1 end

	return t

end



function estimate(x,y,vx,vy,height,typ)

	local collision=1
	local tw,tn,ts=0,0,0
	local tr,xr,yr,vxr,vyr,n=0,0,0,0,0,0

	while(collision==1) do
		ts=collide(x,y,vx,vy,400,316,7)
		if (vx>0)
			then
				tw=time((768.5-x)/vx)
				tn=time((361.5-x)/vx)
			else
				tw=time((31.5-x)/vx)
				tn=time((438.5-x)/vx)
		end
		local th=time(tb(y,vy,height,typ))
		local t=10000

		if ((ts>0) and (ts<t)) then t=ts end
		if (((tn>0) and (tn<t)) and (yb(y,vy,tn)<316)) then t=tn end
		if ((tw>0) and (tw<t)) then t=tw end

		if (th>t)
			then
				if (ts>0)
					then
						tr=tr+t
						x=x+vx*t
						y=yb(y,vy,t)
						vy=vy-g*t
						xr=x
						yr=y
						vxr=vx
						vyr=vy
						n=1
						collision=0
					elseif ((t==tn) or (t==tw))
					then
						tr=tr+t
						x=x+vx*t
						y=yb(y,vy,t)
						vx=-vx
						vy=vy-g*t
						collision=1
				end
			else
				tr=tr+th
				vxr=vx
				vyr=vy-g*th
				xr=x+vx*th
				yr=yb(y,vy,th)
				collision=0
		end
	end
	if (tr<0)
		then
			return -1,-1,-1,-1,-1,-1
		else
			return xr,yr,vxr,vyr,tr,n
	end
end


function impact(x,y,vx,vy,xpos,ypos,targety,jum,move)
	local x1,y1,vx1,vy1=0,0,0,0
	local x2,y2,vx2,vy2,t2,nn=0,0,0,0,0,0
	local xpos1=xpos
	local ypos1=ypos
	for t=0,7,0.2 do
		if (jum==1) then
			ypos1=yp(tp(ypos)+t)
		end
		x1=x+vx*t
		if (x1<31.5) then x1=2*31.5-x1 end
		if (x1>368.5) then x1=2*368.5-x1 end
		y1=yb(y,vy,t)
	
		if ((xpos1-x1)^2+(ypos1+19-y1)^2<(31.5+25)^2) then
			local dx=x1-xpos1
			local dy=y1-(ypos1+19)
			local l=math.sqrt(dx^2+dy^2)
			vx1=dx/l
			vy1=dy/l
			x1=x1+vx1*3
			y1=y1+vy1*3
			vy1=vy1*13.125
			vx1=vx1*13.125
			x2,y2,vx2,vy2,t2,nn=estimate(x1,y1,vx1,vy1,targety,2)
			break
		end
	end
	
	return x2,y2,x1,y1,vx1,vy1
end

function playto(position,delta)

	if (math.abs(est-esto5)>0.2) then

		esto5=est
		imp,impt,impf=0,0,0
		x1=0
		t=27
		typ1=60
		typ2=120
				if (position<400) then
					n1=2
					n2=-3
				elseif (position>600) then
					n1=-4
					n2=-11
				else
					n1=-1
					n2=-8
				end
		while (t>=0) do

			if (t==0) then j=0 else j=1 end
			t1f=t
			y1f=yp(t1f)
			t2g=math.floor(tb(y,vy,y1f+h,2))
			y2f=yb(y,vy,t2g)
			vy2f=vy-g*t2g
			x2f,y2f,vx2f,vy2f,t2f,_=estimate(x,y,vx,vy,y2f-vy2f/30,2)
			t1f=math.floor(tp(y2f-h))
			y1f=yp(t1f)
			if (t1f+math.abs(math.ceil((tp2(posy(),vp))))<t2f) and (t2g>1) and (t2f>1) then

				for x1f=pos(x2f)+n1*4.5,pos(x2f)+n2*4.5,-4.5 do
					impf,_,xaf,yaf,vxaf,vyaf=impact(x2f,y2f,vx2f,vy2f,x1f,y1f,220,j,0)

					if (impf>position-delta) and (math.abs(posx()-x1f)/4.5+1<t2f) and (x1f>=0) and (x1f<=360) and (x2f<=365) and (impf<position+delta) then
						imp=impf
						x1=x1f
						y1=y1f
						t1=t1f
						x2=x2f
						y2=y2f
						t2=t2f
						vx2=vx2f
						vy2=vy2f
						xa=xaf
						ya=yaf
						vxa=vxaf
						vya=vyaf
						break
					end
					if (impf>position+delta) and ((x1f-pos(x2f))/4.5<0) then break end
				end

	
			end
			if (imp>position-delta) and (imp<position+delta) then break end
		if (t>15) then t=t-6
		else t=t-3 end
		end
		t2=t2+1

	end

	if (x1>=0) and (imp>0) then
	t2=t2-1
		move(x1)
		if (t2<=t1+0.1) and (y1>146) then
			 jump()
		end
	else
		if (delta>60) then
			save()
			else
			playto(600,100)
			end
	end
end



function save()

	if (math.abs(est-esto3)>0.2) then
		delta=30
		esto3=est
		imp,impt,impf=0,0,0
		x1=0
		t=33
		typ1=60
		typ2=120

		while (t>=0) do
			if (t==0) then j=0 else j=1 end
			t1f=t
			y1f=yp(t1f)
			t2g=math.floor(tb(y,vy,y1f+h,2))

			y2f=yb(y,vy,t2g)
			vy2f=vy-g*t2g

			x2f,y2f,vx2f,vy2f,t2f,_=estimate(x,y,vx,vy,y2f+0.1,2)

			t1f=math.floor(tp(y2f-h))
			y1f=yp(t1f)
-- 			if (y1f<316-h) then break end
-- 			(t1f+math.abs(math.ceil((tp2(posy(),vp))))<t2f)
			x1f=pos(x2f)

			if (t2g>1) and (t1f+tp2(posy(),vp)+1<t2f) and (math.abs((x1f-posx())/4.5)<t2f) and (x1f<=360) and (x1f>=0) then

				impf,_,xaf,yaf,vxaf,vyaf=impact(x2f,y2f,vx2f,vy2f,x1f,y1f,220,j,0)
				imp=impf
				x1=x1f
				y1=y1f
				t1=t1f
				x2=x2f
				y2=y2f
				t2=t2f
				vx2=vx2f
				vy2=vy2f
				xa=xaf
				ya=yaf
				vxa=vxaf
				vya=vyaf
				t=-5
			end

			if (t>15) then
					t=t-6
				else
					t=t-3
			end
		end
		t2=t2+1
	end

if (t2>-1) and (x1>=0) and (x1<=360) and (imp>0) then
	t2=t2-1
		move(x1)
		if (t2<=t1+0.1) and (y1>146) then
			 jump()
		end
else move(200)
end
end




function attack(low,high)

	if (math.abs(est-esto1)>0.2) then
		h2=900
		esto1=est
		imp,impt,impf=0,0,0
		x1=0
		t=27
		typ1=low--60
		typ2=high--120

		while (t>=0) do
			if (t==0) then j=0 else j=1 end
			t1f=t
			y1f=yp(t1f)
			t2g=math.floor(tb(y,vy,y1f+h,2))
			y2f=yb(y,vy,t2g)
			vy2f=vy-g*t2g
			x2f,y2f,vx2f,vy2f,t2f,_=estimate(x,y,vx,vy,y2f-vy2f/30,2)
			t1f=math.floor(tp(y2f-h))
			y1f=yp(t1f)
			if (t1f+math.abs(math.ceil((tp2(posy(),vp))))+2<t2f) and (t2g>1) and (t2f>1) then
				for x1f=pos(x2f)-20*4.5,pos(x2f)-4*4.5,4.5 do
					impf,_,xaf,yaf,vxaf,vyaf=impact(x2f,y2f,vx2f,vy2f,x1f,y1f,220,j,0)
					h1=yb(yaf,vyaf,(380-xaf)/vxaf)
					h2=yb(yaf,vyaf,(420-xaf)/vxaf)
					height=316+7+31.5

					if (impf>432) and (math.abs(posx()-x1f)/4.5+1<t2f) and (x1f>0) and (x2f<361) and (x1f<330) and (h2>height+typ1) and (h1>height+typ1) and (h2<height+typ2) and (h1<height+typ2)then
						imp=impf
						x1=x1f
						y1=y1f
						t1=t1f
						x2=x2f
						y2=y2f
						t2=t2f
						vx2=vx2f
						vy2=vy2f
						xa=xaf
						ya=yaf
						vxa=vxaf
						vya=vyaf
						break
					end
					if (impf>432) and (h2>height+typ2) then break end
				end

	
			end
			if (imp>0) then break end
		if (t>15) then t=t-6
		else t=t-3 end
		end
		t2=t2+1

	end

	if (x1>0) and (imp>0) then
	t2=t2-1
		move(x1)
		if (t2<=t1+0.1) and (y1>146) then
			 jump()
		end
	else
 		playto(600,100)
	end
end

function wall()

	if (math.abs(est-esto4)>0.2) then
		delta=30
		esto4=est
		imp,impt,impf=0,0,0
		x1=0
		t2=26
		typ1=60
		typ2=120


			t1=t2
			y1=yp(t1)

			t2g=math.floor(tb(y,vy,y1+h,2))
			y2=yb(y,vy,t2g)

			vy2=vy-g*t2g
			x2,y2,vx2,vy2,t2,_=estimate(x,y,vx,vy,y2+vy2/30,2)

			t1=math.floor(tp(y2-h))
			y1=yp(t1)
			x1=pos(x2)-10*4.5


		t2=t2+1

	end

	t2=t2-1
		move(x1)
		if (t2<=t1+0.1) and (y1>146) then
			 jump()
		end

end



function OnOpponentServe()
reset()
	if (math.abs(pos2(posx())-posx())>1) then
			move(400)
		else
			move(200)
	end
end

function OnServe(ballready)
reset()
	if (math.abs(pos2(posx())-posx())>1) then
			move(400)

		else -- -10,-5,+8

			if (s<0.3) then
					b=8
				elseif (s>=0.3) and (s<0.7) then
					b=-10
				elseif (s>=0.6) and (s<1) then
					b=-5

			end
			a=198+b*4.5

			if (math.abs(posx()-a)<2) then
					jump()
				else
					move(a)
			end
	end

end



function OnGame()

			s=math.random()
	yp2=y1p
	y1p=posy()
	vp=y1p-yp2
	
	if (math.abs(imp1-imp)>0.1) then
		imp2=imp1
		imp1=imp
	end
	
	esttold=estt
	netold=netc
	toucho=touch
	touch=touches()
	if (touch<toucho) then p=math.random() 
pp=math.random()
reset()
end
--phase
--0: Spielstop
--1: eigenes Feld
--2: gegnerisches Feld
--3: Ball hängt fest
--4: Netzblock
--5: Netzblock weiterführen
--6: Netzball vorher abfangen

	x,y,vx,vy=ballx(),bally(),bspeedx(),bspeedy()
	estt,yestt,vxestt,vyestt,tnet,netc=estimate(x,y,vx,vy,220)
	if (bally()<140) and (active==1) then
	active=0

	end
	if (math.abs(esttold-estt)<0.1) then -- Schutz gegen Fehler bei der estimate-Funktion während Netz/Wandkollisionen
			est=estt
	end
	if (active==1) then
			if (phase==4) and ((est>438.5) or (est<361.5)) then
				nettime=10
				phase=5
				elseif (phase==5) and (nettime>0) then
					nettime=nettime-1
				else
					if (est>438.5) then
							phase=2
						elseif (est<438.5) and (est>361.5) then
							if (est<400) and (math.abs(bspeedx())<1) then
									phase=99
									wall()
								else
									if (yb(y,vy,(31.5-x)/vx)>220) then
											phase=6
										else
											phase=4
									end
							end
						else
							phase=1
					end
			end
		else
			phase=0
	end

	
	if (math.sqrt((ballx()-400)^2+(bally()-316)^2)<(31.5+7)) and (math.sqrt((bspeedx())^2+(bspeedy())^2)<2) then phase=3 end
--phase
--0: Spielstop
--1: eigenes Feld
--2: gegnerisches Feld
--3: Ball hängt fest
--4: Netzblock
--5: Netzblock weiterführen
--6: Netzball vorher abfangen

	if (phase==2) then
			move(200)
		elseif (phase==1) then
			if (p<0.5) then
					if (touch==0) then
							playto(320,30)
						elseif(touch==1) then
							if (p<0.2) then
									attack(60,120)
								elseif (p>=0.2) and (p<0.25) then
									attack(30,60)
								elseif (p>=0.25) and (p<0.275) then
									playto(420,20)
								elseif (p>=0.275) and (p<0.3) then
									playto(480,40)
								elseif (p>=0.3) and (p<0.38) then
									playto(760,40)
								elseif (p>=0.38) and (p<0.5) then
									playto(380,20)
							end
						elseif (touch==2) then
								if (p<0.4) then
										playto(600,100)
									else
										wall()
								end
					end
				else
					if (touch==0) then
							if (p<0.8) then
									attack(60,120)
								elseif (p>=0.8) and (p<0.9) then
									attack(30,60)
								elseif (p>=0.9) and (p<1) then
									playto(760,40)
							end
						else
							playto(600,100)
					end
			end

		elseif (phase==4) then
			if (tnet<=tp(393)-4) or (nettime>0) then jump() end
			if (math.abs(posx()-360)/4.5+2<=tnet) then
					left()
				else
					right()
			end
		elseif (phase==5) then
			right()
			jump()
		elseif (phase==6) then
-- 			move(200)
			save()
		elseif (phase==3) then
			if (posx()>300) then
				jump()
			end
			right()
	end
	
	if ((x1==0) or (imp==0)) and (phase==1) then 
		move(est)
	end

	if ((x1==0) or (imp==0)) and (phase==6) then 
		move(200)
	end
-- debug(0)
-- debug(est)
-- debug(imp)
end
