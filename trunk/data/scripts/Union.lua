--UNION
--27.3.2007 - version 2
--Enormator
-- 11.1.12 - insert game-provided physic constants where possible - by ngc92

--Startwerte setzen
--set starting values
OOSinit=false
OSinit=false
OGinit=false
decision=0
funcs=4
oldtouches=0
oldbspeedx=0
serve=false

--Weltkonstanten definieren
--define world constants
middle = CONST_FIELD_WIDTH/2
wallright = CONST_FIELD_WIDTH - CONST_BALL_RADIUS
blobbyheadheight = CONST_GROUND_HEIGHT + CONST_BLOBBY_HEIGHT + CONST_BALL_RADIUS
blobbymaxjump = 393.625
blobbygroundpos = CONST_GROUND_HEIGHT + CONST_BLOBBY_HEIGHT / 2 
netheight = CONST_NET_HEIGHT + CONST_NET_RADIUS -- height is just the rod. for total height, we have to add the net-sphere-radius too
netcolissionleft = middle - CONST_NET_RADIUS - CONST_BALL_RADIUS 
netcolissionright = middle + CONST_NET_RADIUS + CONST_BALL_RADIUS

--Hauptfunktionen
--main functions

function OnOpponentServe()
 --initialisiere Funktion
 --initialise function
 OSinit=false
 OGinit=false
 if (OOSinit==false) then --einmal pro Aufruf von OnOpponentServe
                          --one time per OnOpponentServe
  decision=decide(1) --Entscheidung faellen
                     --make a decision
  OOSinit=true
  serve=false
 end
 dofunc (1, decision, true) --Entscheidung ausfuehren
                            --follow the taken decision
end

function OnServe(ballready)
 --initialisiere Funktion
 --initialise function
 OOSinit=false
 OGinit=false
 if (OSinit==false) then --einmal pro Aufruf von OnServe
                         --one time per OnServe
  decision=decide(2) --Entscheidung faellen
                     --make a decision
  OSinit=true
  serve=true --Aufschlag auch waehrend OnGame weitermachen
             --continue serve while OnGame
 end
 dofunc (2, decision, true) --Entscheidung ausfuehren
                            --follow the taken decision
end

function OnGame()
 --initialisiere Funktion
 --initialise function
 OOSinit=false
 OSinit=false
 if (OGinit==false) and (serve==false) then --einmal pro Aufruf von OnGame und wenn der Aufschlag schon vorbei ist
                                            --one time per OnGame and when serve is already over
  decision=decide(3) --Entscheidung faellen
                     --make a decision
  OGinit=true
 end
 if (serve==true) then --Falls noch beim Aufschlag
                       --if still while serve
  dofunc (2, decision, true) --Der Funktion sagen, dass sie immernoch beim Aufschlag ist
                             --tell the function, it's still OnServe
 else
  dofunc (3, decision, true) --Der Funktion sagen, dass sie im Spiel ist
                             --tell the function that it's OnGame
 end
 if (ballx()>netcolissionleft+CONST_BALL_RADIUS) then --Falls Ball nicht mehr erreichbar
                                               --If Ball not gettable by Bot
  serve=false --Aufschlagende erzwingen
              --Make an end to the serve
 end
 if (touches()~=oldtouches) or (math.abs(oldbspeedx)~=math.abs(bspeedx())) then --Hat sich die Situation geändert
                                                                                --If the situation has changed
  OGinit=false --gleich Entscheidung neu fällen
               --redo decision
  oldtouches=touches()
  oldbspeedx=bspeedx()
 end
end

--Entscheidungsfunktionen
--decision functions

function decide (funcno)
 t1=1
 chosen=1
 chosenret=0
 --Alle Funktionen abfragen und die mit dem größten Return wählen
 --test all functions and take the one with the highest return value
 while (t1 <= funcs) do
  temp=dofunc(funcno,t1,false)
  if (temp > chosenret) then
   chosen=t1
   chosenret=temp
  end
  t1=t1+1
 end
 debug (chosenret) --Sagt, für wie gut sich die gewählte Funktion hält
                   --tells how good the chosen function says to fit
 return chosen
end

function dofunc (funcno, actionno, action) --Weist jeder Aktionsnummer den echten Funktionsnamen zu
                                           --converts actionnumbers to the real names of the functions
 if (actionno==1) then
  return std45deg (funcno, action)
 end
 if (actionno==2) then
  return takelow (funcno, action)
 end
 if (actionno==3) then
  return wait (funcno, action)
 end
 if (actionno==4) then
  return twohitserve1 (funcno, action)
 end
 return false
end

--Ausführbare Funktionen
--executable functions

function std45deg (funcno, action) --spielt Ball aus der Luft bei maxjump im 45° Winkel an
                                   --plays ball in the air at height maxjump with 45° angle
                                   --funcno(s)=2,3
 maxjump  = blobbymaxjump
 distance = 32.25
 targetx=estimatex (maxjump) - distance
 if (funcno==1) then
  return -1
 end
 if (action==false) then
  if (funcno==2) and (action==false) then
   return math.random(10, 100)
  end
  if (funcno==3) and (action==false) then
   if (bspeedx() <= 3) and (math.max(balltimetox(targetx),balltimetoy(maxjump)) >= math.max(blobtimetoy(maxjump), blobtimetox(targetx))) then
    if (bspeedx()==0) then
     ret=85
    else
     ret=math.min((10^(-math.abs(bspeedx()))+1),1)*85
    end
    if (estimhitnet()==true) and (blobtimetox(netcolissionleft)<=balltimetoy(netheight)) then
     ret=190
    end
    return ret
   else
    return 0
   end
  end
 end
 if (action==true) then
  if (funcno==2) then
   if (posy()==144.5) then
    moveto (targetx)
   end
   if (math.abs(posx()-targetx)<4.5) and (ballx()==200) and (bally()==299.5) then
    jump()
   end
  end
  if (funcno==3) then
   moveto (targetx)
   if ((bally()<580) and (bspeedy()<0)) then
    jump()
   end
  end
  if (jumpcase) then
   jump()
  end
 end
end

function takelow (funcno, action) --Ballannahme ohne Sprung zum Stellen (bspeedx()=0) selten exakt
                                  --take ball without jump to create a ball with bspeedx()=0 (sellen exactly)
                                  --funcno(s)=3
 if (funcno==1) or (funcno==2) then
  return -1
 end
 if (action==false) then
  if (touches()<=1) then
   return 1
  else
   return -1
  end
 end
 if (action==true) then
  moveto (estimatex(220.5))
 end
end

function wait (funcno, action) --Auf guter Position auf eine gegnerische Aktion warten
                               --wait for an opponent's move at a good position
                               --funcno(s)=1,3
 if (funcno==2) then
  return -1
 end
 if (funcno==1) and (action==false) then
  return 200
 end
 if (funcno==3) and (action==false) then
  if (estimatex(393.625) > 424.5) then
   return 200
  else
   return -1
  end
 end
 if (action==true) then
  moveto (180)
 end  
end

function twohitserve1 (funcno, action) --Aufschlag: Stellen (bspeedx()=0), rueber
                                       --serve: up (bspeedx()=0), play
                                       --funcno(s)=2
 if (funcno==1) or (funcno==3) then
  return -1
 end
 if (action==false) then
  return math.random(10,100)
 else
  if (touches()==0) then
   moveto (200)
   if (math.abs(posx()-200)<5) then
    jump()
   end
  end
  if (touches()==1) then
   moveto (estimatex(blobbymaxjump)-45)
   if (bally()<580) and (bspeedy()<0) then
    jump()
   end
  end
  if (touches()==3) then
   serve=false
  end
 end
end

--mathematische Hilfsfunktionen
--mathematical helpers

function estimatex(destY) --gibt möglichst genaue Angabe der X-Koordinate zurück, bei der sich der Ball befindet, wenn er sich bei der angegebenen Y Koordinate befindet
                          --returns exact ballx when bally will be given destY
 if (bspeedy()==0) and (bspeedx()==0) then
  return ballx()
 end
 time1 =(-bspeedy()-math.sqrt((bspeedy()^2)-(2*CONST_BALL_GRAVITY * (bally()-destY)))) / CONST_BALL_GRAVITY
 resultX = (bspeedx() * time1) + ballx()
 estimbspeedx=bspeedx()

 if(resultX > wallright) then -- Korrigieren der Appraller an der Rechten Ebene
  resultX = 2 * CONST_FIELD_WIDTH - resultX
  estimbspeedx=-estimbspeedx
 end

 if(resultX < CONST_BALL_RADIUS) then -- korrigieren der Appraller an der linken Ebene
  resultX = 2 * CONST_BALL_RADIUS - resultX
  estimbspeedx=-estimbspeedx
 end

 if (resultX > netcolissionleft) and (estimatey(netcolissionleft-CONST_BALL_RADIUS) <= netheight) and (estimbspeedx > 0) then
  resultX = 2 * netcolissionleft - resultX
  estimbspeedx=-estimbspeedx
 end
 return resultX
end

function balltimetox (x) --Zeit in Physikschritten, die der Ball bei der derzeitigen Geschwindigkeit zu der angegebenen X-Position braucht
                         --time in physic steps, which the ball needs to reach the given x-position with momentany speed
 if (bspeedx() == 0) then
  return 10000
 end
 strecke=x-ballx()
 time=(strecke)/bspeedx()
 return time
end

function blobtimetoy (y) --Zeit, die ein Blob braucht, um eine Y Position zu erreichen
                         --time needed by a blob to reach a given y coordinate
 if (y>383) then
  y=383
 end
 grav = CONST_BLOBBY_GRAVITY / 2    -- half, because we use jump buffer
 time1 = CONST_BLOBBY_JUMP/grav + math.sqrt(2*grav*(y-blobbygroundpos) + CONST_BLOBBY_JUMP*CONST_BLOBBY_JUMP) / grav
 time2 = CONST_BLOBBY_JUMP/grav - math.sqrt(2*grav*(y-blobbygroundpos) + CONST_BLOBBY_JUMP*CONST_BLOBBY_JUMP) / grav
 timemin=math.min(time1,time2)
 return timemin
end

function estimhitnet() --Wird der Ball das Netz treffen (bool)
                       --Will the ball hit the net (bool)
 safety=5
 if (361.5-safety < estimatex(323)) and (estimatex(323) < 438.5+safety) then
  answer=true
 else
  answer=false
 end
 return answer
end

function estimatey (x) --Y Position des Balls, wenn er sich an der angegebenen X Koordinate befindet
                       --y position of the ball, when it is at the given x coordinate
 y=ballyaftertime(balltimetox(x,3))
 return y
end

function ballyaftertime (t) --Y Position des Balls nach der angegebenen Zahl von Physikschritten
                            --y position of the ball after the given time
 y=1/2*CONST_BALL_GRAVITY*t^2+bspeedy()*t+bally()
 return y
end

function blobtimetox (x) --Zeit, die der Bot benoetigt, um eine gewisse X-Koordinate zu erreichen
                         --time needed for the bot to reach a given x-coordinate
 time=math.abs(posx()-x)/4.5
 return time
end

function balltimetoy (y) --Zeit, die der Ball bis zu einer Y Position benoetigt
                         --time needed by the ball to reach a given y position
 time1=-bspeedy()/CONST_BALL_GRAVITY+1/CONST_BALL_GRAVITY*math.sqrt(2*CONST_BALL_GRAVITY*(y-bally())+bspeedy()^2)
 time2=-bspeedy()/CONST_BALL_GRAVITY-1/CONST_BALL_GRAVITY*math.sqrt(2*CONST_BALL_GRAVITY*(y-bally())+bspeedy()^2)
 timemax=math.max(time1, time2)
 return timemax
end