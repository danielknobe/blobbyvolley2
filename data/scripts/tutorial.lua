function OnOpponentServe()
moveto(120)
end

function OnServe(ballready)
serveTarget = ballx()-48
if (posx() < serveTarget + 2) and (posx() > serveTarget - 2) then
if ballready then
jump()
end
readyToServe = true
else
readyToServe = false
if posx() < serveTarget then
right()
else
left()
end
end
end


function OnGame()
moveto(estimate())

end
