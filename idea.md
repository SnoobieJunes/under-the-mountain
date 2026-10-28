# Under the Mountain — Dragon Simulator (refined 2026-10-27, T-001)

> You are the dragon. Smell everything. Trust nothing.
> Normative plan: docs/PLAN.md (phases, non-goals, gates). This file is the design vision.

## Pitch
You are a dragon with a hoard, a lair under a mountain, and a problem: you can
smell *all* treasure — yours, theirs, everyone's — as golden streams in the air,
but smell tells you value, never distance. Patrolling is your job. Thieves know it.

## Core loop
Smell (scent streams, thickness = value) → patrol the lair → catch thieves →
choose: kill inside (safe) / chase outside (Notoriety ★↑) / roar-scare (loot
dropped, thief flees and talks) → eat to restore stamina → defend the next,
harder wave — or fly out and become the problem yourself (livestock, villages,
crown jewels) and pay for it in Notoriety.

## Systems (full vision — delivery phases in PLAN.md)
- **Scent:** streams connect you to every treasure; bigger value = thicker stream. No range/room info, ever.
- **Thieves:** solo burglars (quiet, fast), dwarf/human parties (traps, slows, escapes), adventurers (scare-able). Each carries loot worth stealing back.
- **Notoriety:** invisible inside the lair; public kills/eaten livestock raise it; higher = more + better raiders, then dragonslayers, then an army. Decays by sleeping a generation away.
- **Flight:** flap to climb, dive for speed, fire/roar from the air, dodge projectiles, hard landings damage you. Movement drains stamina; zero stamina drains HP.
- **Resources:** HP / magic / stamina. Heal by magic, artifacts, or eating (wildlife < livestock < people in Notoriety cost).
- **Hoard & artifacts:** treasure is score; artifacts are wearable power (Seeing Stone: true sight vs smell; hardened scales; pendant; mind-control crown; elemental rings; thief-glow helm) that a escaping thief can steal from you.
- **Progression:** XP from kills + treasure, dragon levels, tribute from townsfolk/zealots at the right reputation.
- **World:** rival dragons, settlements, castles with crown jewels — the open world the scent streams point into.
- **Intro:** you wake buried in your hoard as a gnome steals in front of you. "WHO DARES ENTER MY LAIR?" — let him go (he talks) or chase (first Notoriety choice). The tutorial is the dilemma.

## Controls
Keyboard/mouse + gamepad, both first-class (flight on sticks must feel right).

## Art direction (as decided 2026-10-27)
Style B — grounded realism, texture-carried dragon (docs/PLAN.md → Art direction).

## Tech (as decided 2026-10-27)
Unreal Engine 5.8.4 (native Apple Silicon/Metal), Meshy for models, Blender
fallback for the dragon rig, Hermes/Muse as the agentic coding harness,
unreal-mcp for live-editor work. Details: docs/ASSET-PIPELINE.md, AGENTS.md.

---

## Original brain-dump (verbatim, pre-refinement — kept for the record)

**Dragon Simulator**
You are a dragon, you have a hoard of treasure and a large lair.  You are able to ‘smell’ treasure which shows up as a gold floating stream in the air and it connects to every piece of treasure in the game.  You can smell it ALL.  The more valuable the treasure, the larger the scent.  The gotcha is that you can’t tell if the treasure is in the other room, or if its on the other side of the world.  So you are constantly moving from room to room checking on your treasure to ensure it hasn’t been stolen.   There are different thieves that will enter your lair, some are burglars like hobbits, others are parties of dwarves or humans and have traps / other was to slow, entrap or escape you, others are just adventurers and can be scared into dropping the treasure.  If you eat or kill intruders inside your lair, no one knows and you are fine.  It’s kinda like they asked for it, but if they get outside of the lair onto the open road or in the world, and the dragon comes out of its lair to kill them, even if they stole from it, it will gain notoriety.  This will cause more adventurers and thieves to descend upon the mountain, thus making it harder to keep your treasure (but also bringing more treasure to you - since the adventurers have magical artifacts and weapons of value on them).   As a dragon, you are free to leave the lair and follow any of the scents for treasure, the world is filled with it.  Including other kingdoms or crowned jewels that are outlaying towns/cities/castles.  It is possible to also just go full evil dragon and try destroy all the people but your “notoriety or infamy” level reaches a certain threshold, then dragonslayers will start showing up to try to kill you and they will be difficult.  Then if you keep killing without letting your level fall (which prolly involves having most of your treasure plundered) the countryside will band together and send an army which will be almost impossible to survive.   The dragon can lower its infamy by sleeping, since dragons can sleep for an entire generation and then those grievances become nothing but stories of their parents.  

Your dragon should start at level 1, and gain experience by killing people and gaining treasure.  The dragon has artifacts and magical equipment that it can wear.  These include a magic pendant, hardened scales, claws, and other magical objects to enhance it.  This is part of the reason holding onto treasure is important because you can lose something a magical seeing stone that allows you to search out treasure and “see it” not just smell it.

The dragon has health, magic and stamina as its resources.  The health will deteriorate and require the dragon to either use magic to heal itself or to use a magical item or to eat meat.  The meat can be wildlife or goats, humans, horses.  Basically any type of wildlife but again it will cause them to gain notoriety if they eat to many farmers animals or eat the humans themselves.

Dragon Simulator You are a dragon, you have a hoard of treasure and a large lair. You are able to ‘smell’ treasure which shows up as a gold floating stream in the air and it connects to every piece of treasure in the game. You can smell it ALL. The more valuable the treasure, the larger the scent. The gotcha is that you can’t tell if the treasure is in the other room, or if its on the other side of the world. So you are constantly moving from room to room checking on your treasure to ensure it hasn’t been stolen.   There are different thieves that will enter your lair, some are burglars like hobbits, others are parties of dwarves or humans and have traps / other was to slow, entrap or escape you, others are just adventurers and can be scared into dropping the treasure. If you eat or kill intruders inside your lair, no one knows and you are fine. It’s kinda like they asked for it, but if they get outside of the lair onto the open road or in the world, and the dragon comes out of its lair to kill them, even if they stole from it, it will gain notoriety. This will cause more adventurers and thieves to descend upon the mountain, thus making it harder to keep your treasure (but also bringing more treasure to you - since the adventurers have magical artifacts and weapons of value on them).
  As a dragon, you are free to leave the lair and follow any of the scents for treasure, the world is filled with it. Including other kingdoms or crowned jewels that are outlaying towns/cities/castles. It is possible to also just go full evil dragon and try destroy all the people but your “notoriety or infamy” level reaches a certain threshold, then dragonslayers will start showing up to try to kill you and they will be difficult. Then if you keep killing without letting your level fall (which prolly involves having most of your treasure plundered) the countryside will band together and send an army which will be almost impossible to survive. The dragon can lower its infamy by sleeping, since dragons can sleep for an entire generation and then those grievances become nothing but stories of their parents.

Your dragon should start at level 1, and gain experience by killing people and gaining treasure. The dragon has artifacts and magical equipment that it can wear. These include a magic pendant, hardened scales, claws, and other magical objects to enhance it. This is part of the reason holding onto treasure is important because you can lose something a magical seeing stone that allows you to search out treasure and “see it” not just smell it.  The townspeople will pay tribute to the dragon.  Certain religious zealots will also pay tribute to the dragon with magical items.    There is a resource constraint with the game because you basically are going to want to be marching around the dragon lair and ensuring that none of your treasure is being taken, while that causing you to lose stamina and need food.  So a careful dragon can actually just survive by eating the burglars and not moving much.  The problem is that some of the burglars may get a way.  

A major part of the game is flight, the dragon flight must feel engaging and exciting similar to how WoW dragon flight dragons fly.  Using accurate physics, requiring the dragon the beat its wings to fly higher, while diving to increase speed.  The dragon can take damage if it lands too hard and you will need to slow your decent by beating its wings or angling down slowly.  A dragon can breath fire and do other attacks from the air.  When in combat the dragon will have to dodge arrows and other projects

Example of dragon flight : [https://www.youtube.com/watch?v=0QjFZPU38U4](https://www.youtube.com/watch?v=0QjFZPU38U4)



[https://x.com/rewind02/status/2107063404691132663](https://x.com/rewind02/status/2107063404691132663) **<- build process with unreal**

Artifact ideas:
 - magic helm that lets you see valuable treasure
* Armor that makes you stronger
* A magic crown that allows you to control the mind of the lesser beings and guard for you
* Magic rings that increase power, add flame abilities, or ability to control other forms of magic or elements
* Glow like sting when thieves are nearby

⠀

Technicals: 
Using the unreal engine, unreal MCP and all the tools we configured for that in Hermes
Using meshy to generate our models It must be built using Metal 4, so that it runs best on apple silicon.  
Have both controller and mouse/keyboard support
Ability to train AI using JEV decision model

Initial gameplay:  Other dragons exist in the world, some are neutral and won’t attack you but others will.  Sometimes dragons will find out about your hoard and come try to take your lair / treasure from you by fighting you.  These dragons usually only appear when you are outside of your lair flying around and making yourself notorious.  Other players can also play dragons in the multiplayer mode.  However in the story mode, similar to Metroid prime and other games you start with “full power” and a huge dragon horde with a bunch of magical artifacts.  These artifacts add magical power, health, and essentially power you up to super strong.  You start with a bunch of treasure and its a tutorial teaching the player how to play.  As you start the game, you are presented with an enemy stealing from right in front of you.  Like Smaug in the Hobbit you come up from slumber sleeping completely covered in your treasure and the gnome in front of you doesn’t acutally see you until its too late.  You roar “WHO DARES ENTER MY LAIR? THAT DOES NOT BELONG TO YOU” In a deep booming draconic voice.  The gnome freaks out and drops the treasure saying he was just an adventure and didn’t know, starts to run out of the lair empty-handed and slips back through a crack.  You get presented with a choice: A) Let the creature leave, but he will surely tell others of your riches and the secret way in.  B) chase him down and kill him, but that will cause you to gain notoriety (think GTA wanted level) and the higher the stars, the countryside will rise up against you.  This is a fundamental choice you face in the game.  As you move or take off and fly up out of the top of the mountain to give chase, your stamina will start draining.  IT informs you that as you move you drain your stamina, which at zero will drain your HP.  So you need to eat.  You can either eat wildlife by flying around, or go eat farmers animals, or eat the thieves themselves.  Again all have varying levels of notoriety, after all the farmer doesn’t like its flock getting torched.  The game is then open world and you can either go on a rampage, or try to go find more treasure or just run around room to room protecting your treasure.  As the dragon you can never see all your treasure at once, its spread through out your lair which is like Smaug’s lair.  A multichambered, multi-layered mountain compound built by dwarves and filled with riches.  If you decide to go out and torch the countryside, to get your notoriety at maximum, then the surrounding villages and castles (think middle earth style world) will raise an army of humans, elves, dwarves, gnomes and march on your compound in a grand battle that you will almost certainly lose because of how many enemies there are, but you can outlast for a while if you are skilled at the combat and movement.  If you go the other route and try to minimize your notoriety but scare away the thieves and gain more treasure, a large black dragon will come down and be like a massive boss battle.  After all there’s always a bigger dragon.  Either way when you “lose” you don’t actually die.  You slink away wonded to a smaller lair without any treasure and basically start at level 1.  Where before this you were “maximum power” showing you what it looks like when you get higher into the game.  You have swipes with claws, tail swipes, fire breath, magical powers and the ability to just crush people by running on them
 