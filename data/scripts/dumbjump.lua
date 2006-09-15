function OnOpponentServe()
  moveto(250)
end

function OnServe(ballready)
  if ballready then
    moveto(ballx() - 45)
    if (posx() < ballx() - 40 and posx() > ballx() - 45) then
      jump()
    end
  else
    moveto(200)
  end
end

function OnGame()

  if bspeedx() < -8 and ballx() < 500 and ballx() > 320 then

    jump()
    left()

  else

    if 400 > ballx() then
      
      if bspeedx() <= 1 then

        moveto(ballx()-25)
      
      end

      if bspeedx() > 1 then

        moveto(ballx() - 35)

      end

      if bally() < 530 and bspeedx() > 4 or bspeedx() < -4 then

        if bally() > 350 then

          jump()

        end


      end

    end

  end

end
