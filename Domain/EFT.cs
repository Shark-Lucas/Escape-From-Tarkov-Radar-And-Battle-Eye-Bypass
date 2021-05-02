using ExtProject.MemoryManagement;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using UnityEngine;

namespace ExtProject.Domain
{
    // I have no idea why this was ever in the domain, wtf
    class EFT
    {
        static ulong imageBase = Memory.Read<UInt64>(Memory.ImageBase());
        static ProcessMethods processMethods = new ProcessMethods();
        static Player localPlayer;

        static Bitmap localPlayerImg = new Bitmap(25, 25);
        static Bitmap enemyImg = new Bitmap(25, 25);
        static Bitmap itemImg = new Bitmap(25, 25);

        static readonly Vector3 center = new Vector3(500, 0, 500);
        static readonly Vector3 scale2 = new Vector3(5f, 1, 5f);
        static readonly Regex rgx = new Regex("[^a-zA-Z0-9 -]");
        static readonly Font font = new Font("Arial", 17, FontStyle.Regular, GraphicsUnit.Pixel);
        static readonly Pen pen = new Pen(Brushes.Black, 1.0f);

        #region cachedVariables  
        private static ulong verticalRecoil = 0;
        private static ulong horizontalRecoil = 0;
        private static ulong breathing = 0;
        private static ulong walk = 0;
        private static ulong step = 0;
        private static ulong motion = 0;

        private static Boolean status = false;
        #endregion  

        public static void FindPlayers(object sender, PaintEventArgs e)
        {
            var gLocal = System.Drawing.Graphics.FromImage(localPlayerImg);
            gLocal.Clear(System.Drawing.Color.Green);
            using (var grf = System.Drawing.Graphics.FromImage(localPlayerImg))
            {
                using (Brush brsh = new SolidBrush(System.Drawing.Color.Black))
                {
                    grf.FillPolygon(brsh, new Point[] { new Point(0, 20), new Point(25, 0), new Point(-10, 0) });
                }
            }

            var gOther = System.Drawing.Graphics.FromImage(enemyImg);
            gOther.Clear(System.Drawing.Color.Red); 
            using (var grf = System.Drawing.Graphics.FromImage(enemyImg))
            {
                using (Brush brsh = new SolidBrush(System.Drawing.Color.Black))
                {
                    grf.FillPolygon(brsh, new Point[] { new Point(0, 20), new Point(25, 0), new Point(-10, 0) });
                }
            }

            var gItems = System.Drawing.Graphics.FromImage(itemImg);
            gItems.Clear(System.Drawing.Color.Blue);

            var status = processMethods.Init();
            if (status)
            {
                ulong gWorld = 0;
                ulong pArrayObj = 0;
                int pArrayLen = 0;

                var gWorldObjects = FindActiveObjects("GameWorld", 3000);

                foreach (var gWorldObject in gWorldObjects)
                {
                    gWorld = Memory.ptrChain(gWorldObject, 0x30, 0x18, 0x28);
                    pArrayObj = Memory.Read<UInt64>(gWorld + 0x78); 
                    pArrayLen = Memory.Read<Int32>(pArrayObj + 0x18); 
                    if (pArrayLen > 0)
                    {
                        if (pArrayLen > 0x300)
                            continue;

                        break;
                    }
                }

                var valItems = FindLoot(gWorld);

                var pArrayBase = Memory.Read<UInt64>(pArrayObj + 0x10) + 0x20; 
                List<Player> players = new List<Player>();

                for (uint i = 0; i < pArrayLen; i++)
                {
                    var entityAddress = Memory.Read<UInt64>(pArrayBase + i * 0x8);
                    var entity = new Player(entityAddress);

                    if (!players.Any(player => player.Address == entityAddress)) { 
                        var pProfile = Memory.Read<UInt64>(entity.Address + 0x3D0); 
                        var pInfo = Memory.Read<UInt64>(pProfile + 0x28); 
                        var pId = Memory.Read<UInt64>(pProfile + 0x18);
                        var regDate = Memory.Read<Int32>(pId + 0x54);
                        entity.id = Memory.ReadUnicodeString(pId + 0x14, 64);

                        var pNickname = Memory.Read<UInt64>(pInfo + 0x10);
                        entity.name = Memory.ReadUnicodeString(pNickname + 0x14, 64);

                        players.Add(entity);
                    }
                }

                foreach (var player in players) {
                    var boneMatrix = Memory.ptrChain(player.Address, new uint[] { 0xA0, 0x28, 0x28, 0x10 });
                    var rootTransform = Memory.ptrChain(boneMatrix, new uint[] { 0x20, 0x10, 0x38 });
                    var playerPosition = Memory.Read<Vector3>(rootTransform + 0xB0);
                    player.location = playerPosition;
                }

                e.Graphics.DrawLine(pen, new Point(0, 500), new Point(1000, 500));
                e.Graphics.DrawLine(pen, new Point(500, 0), new Point(500, 1000));
                e.Graphics.DrawEllipse(pen, 500f - 250, 500f - 250, 500, 500);

                TextRenderer.DrawText(e.Graphics, "50M", font, new Point(228, 500), System.Drawing.Color.Black);
                TextRenderer.DrawText(e.Graphics, "50M", font, new Point(750, 500), System.Drawing.Color.Black);
                TextRenderer.DrawText(e.Graphics, "50M", font, new Point(500, 235), System.Drawing.Color.Black);
                TextRenderer.DrawText(e.Graphics, "50M", font, new Point(500, 750), System.Drawing.Color.Black);

                foreach (Player p in players)
                {
                    if (p.id.Contains("3131624"))
                    {
                        localPlayer = p;
                        recoil(p);

                        System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(localPlayerImg);
                        e.Graphics.DrawImage(RotateImage(localPlayerImg, localPlayer.GetDirection()), new Point((int)center.x - (localPlayerImg.Width / 2), (int)(center.z) - (localPlayerImg.Height / 2)));
                        TextRenderer.DrawText(e.Graphics, "You", font, new Point((int)center.x, (int)center.z), System.Drawing.Color.Black);
                        double x = center.x + 500 * Math.Cos(Deg2Rad(-90 + localPlayer.GetDirection() + 45));
                        double y = center.y + 500 * Math.Sin(Deg2Rad(-90 + localPlayer.GetDirection()));
                    }
                }

                if (players == null)
                {
                    return;
                }

                foreach (Item item in valItems)
                {
                    try
                    {
                        Vector3 temp = (item.location - localPlayer.location);
                        Vector3 k = new Vector3(1, 1, -1);
                        temp = Vector3.Scale(temp, k);
                        Vector3 temp1 = Vector3.Scale(temp, scale2);
                        Vector3 temp2 = temp1 + center;

                        e.Graphics.DrawImage(itemImg, new Point((int)temp2.x - (localPlayerImg.Width / 2), (int)(temp2.z) - (localPlayerImg.Height / 2)));
                        TextRenderer.DrawText(e.Graphics, item.name, font, new Point((int)temp2.x, (int)temp2.z), System.Drawing.Color.Black);
                        TextRenderer.DrawText(e.Graphics, "Distance " + Vector3.Distance(localPlayer.location, item.location), font, new Point((int)temp2.x, (int)temp2.z + 25), System.Drawing.Color.Black);
                    }
                    catch
                    {

                    }
                }

                foreach (Player p in players)
                {
                    try
                    {
                        if (p.name.Equals(localPlayer.name))
                        {
                            continue;
                        }

                        if (p.location.x == 0 && p.location.z == 0)
                        {
                            continue;
                        }

                        Vector3 temp = (p.location - localPlayer.location);
                        Vector3 k = new Vector3(1, 1, -1);
                        temp = Vector3.Scale(temp, k);
                        Vector3 temp1 = Vector3.Scale(temp, scale2);
                        Vector3 temp2 = temp1 + center;

                        if (p.name.Contains("ALLY NAME"))
                        {
                            e.Graphics.DrawImage(RotateImage(localPlayerImg, p.GetDirection()), new Point((int)temp2.x - (localPlayerImg.Width / 2), (int)(temp2.z) - (localPlayerImg.Height / 2)));
                        }
                        else
                        {
                            e.Graphics.DrawImage(RotateImage(enemyImg, p.GetDirection()), new Point((int)temp2.x - (localPlayerImg.Width / 2), (int)(temp2.z) - (localPlayerImg.Height / 2)));
                        }

                        TextRenderer.DrawText(e.Graphics, p.name, font, new Point((int)temp2.x, (int)temp2.z), System.Drawing.Color.Black);
                        TextRenderer.DrawText(e.Graphics, "Distance " + Vector3.Distance(localPlayer.location, p.location), font, new Point((int)temp2.x, (int)temp2.z + 25), System.Drawing.Color.Black);
                    }
                    catch
                    {

                    }
                }
            }
        }
        private static List<Item> FindLoot(ulong gWorld)
        {
            var itemList = Memory.Read<UInt64>(gWorld + 0x58);
            var itemsCount = Memory.Read<Int32>(itemList + 0x18);

            var actItem = Memory.Read<UInt64>(itemList + 0x10);
            actItem = Memory.Read<UInt64>(actItem + (0x20 + 0x8));

            var firstItem = actItem;
            var ret = new List<Item>();

            var i = 0;
            do
            {
                i++;
                if (i > itemsCount)
                    break;

                var unknownPtr = Memory.Read<UInt64>(actItem + 0x10);
                var interactiveClass = Memory.Read<UInt64>(unknownPtr  + 0x28);
                var baseObject = Memory.Read<UInt64>(interactiveClass  + 0x10);
                var gameObject = Memory.Read<UInt64>(baseObject + 0x30);
                var pGameObjectName = Memory.Read<UInt64>(gameObject + 0x60);
                var itemName = Memory.ReadGenericType<String>(pGameObjectName, 128);

                if (!IsLootableObject(itemName))
                {
                    itemName = rgx.Replace(itemName, "");
                    if (isValuableItem(itemName))
                    {
                        var newItem = new Item(actItem);

                        var item = Memory.Read<UInt64>((interactiveClass + 0x50));
                        var itemTemplate = Memory.Read<UInt64>((item + 0x40));
                        var pItemIdStr = Memory.Read<UInt64>((itemTemplate + 0x50));
                        var itemId = Memory.ReadUnicodeString(pItemIdStr, 128);

                        var itemDict = itemDictionary();
                        foreach (var jsonEntry in itemDict)
                        {
                            var cleanName = jsonEntry.Key.Split('.')[1];
                            if (itemId.Contains(cleanName))
                            {
                                newItem.name = (string)jsonEntry.Value;
                            }
                        }

                        var objectClass = Memory.Read<UInt64>((gameObject + 0x30));
                        var pointerToTransform_1 = Memory.Read<UInt64>((objectClass + 0x8));
                        var pTransform = Memory.Read<UInt64>((pointerToTransform_1 + 0x38));
                        newItem.location = Memory.Read<Vector3>(pTransform + 0xB0);

                        if (newItem.location != new Vector3())
                        {
                            ret.Add(newItem);
                        }
                    }
                }
                else
                {
                    var newItem = new Item(actItem);

                    var objectClass = Memory.Read<UInt64>((gameObject + 0x30));
                    var pointerToTransform_1 = Memory.Read<UInt64>((objectClass + 0x8));
                    var pointerToTransform_2 = Memory.Read<UInt64>((pointerToTransform_1 + 0x28));
                    var pTransform = Memory.Read<UInt64>((pointerToTransform_2 + 0x10));
                    newItem.location = Memory.Read<Vector3>(pTransform + 0xB0);
                }

                ulong index = (ulong)(0x8 * i);
                actItem = Memory.Read<UInt64>(itemList + 0x10);
                actItem = Memory.Read<UInt64>(actItem + (0x20 + index));
            } while (firstItem != itemList);

            return ret;
        }
        private static Bitmap RotateImage(Bitmap bmp, float angle)
        {
            Bitmap rotatedImage = new Bitmap(bmp.Width, bmp.Height);
            using (System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(rotatedImage))
            {
                g.TranslateTransform(bmp.Width / 2, bmp.Height / 2);
                g.RotateTransform(angle + 45);
                g.TranslateTransform(-bmp.Width / 2, -bmp.Height / 2);
                g.DrawImage(bmp, new Point(0, 0));
            }

            return rotatedImage;
        }
        static double Deg2Rad(double deg)
        {
            return (Math.PI / 180) * deg;
        }

        static void recoil(Player p)
        {
            byte[] buff = new byte[sizeof(float) * 3];
            Buffer.BlockCopy(BitConverter.GetBytes(0f), 0, buff, 0 * sizeof(float), sizeof(float));
            Buffer.BlockCopy(BitConverter.GetBytes(0f), 0, buff, 1 * sizeof(float), sizeof(float));

            if (verticalRecoil == 0 || horizontalRecoil == 0 || breathing == 0 || walk == 0 || step == 0 || motion == 0) { 
                verticalRecoil = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x48 }) + 0x38;
                horizontalRecoil = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x48 }) + 0x40;
                breathing = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x28 }) + 0xA4;
                walk = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x30 }) + 0x44;
                step = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x30 }) + 0x40;
                motion = Memory.ptrChain(p.Address, new uint[] { 0x168, 0x38 }) + 0xD0;
            }

            Memory.WriteBytes(verticalRecoil, buff);
            Memory.WriteBytes(horizontalRecoil, buff);
            Memory.WriteBytes(breathing, buff);
            Memory.WriteBytes(walk, buff);
            Memory.WriteBytes(step, buff);
            Memory.WriteBytes(motion, buff);
        }

        static bool isValuableItem(String itemName)
        {
            return itemName.ToLowerInvariant().Contains("weapon")
                        || itemName.ToLowerInvariant().Contains("card")
                        || itemName.ToLowerInvariant().Contains("info")
                        || itemName.ToLowerInvariant().Contains("illuminator")
                        || itemName.ToLowerInvariant().Contains("tetriz")
                        || itemName.ToLowerInvariant().Contains("bitcoin")
                        || itemName.ToLowerInvariant().Contains("phased")
                        || itemName.ToLowerInvariant().Contains("vpx")
                        || itemName.ToLowerInvariant().Contains("virtex")
                        || itemName.ToLowerInvariant().Contains("reap")
                        || itemName.ToLowerInvariant().Contains("flir")
                        || itemName.ToLowerInvariant().Contains("corpse")
                        || itemName.ToLowerInvariant().Contains("armor")
                        || itemName.ToLowerInvariant().Contains("thermal")
                        && !itemName.ToLowerInvariant().Contains("sanitar");
        }

        static bool IsLootableObject(String gameObjectName)
        {
	        if (gameObjectName.Contains("cap"))
		        return true;
	        if (gameObjectName.Contains("Ammo_crate_Cap"))
		        return true;
	        if (gameObjectName.Contains("Grenade_box_Door"))
		        return true;
	        if (gameObjectName.Contains("Medical_Door"))
		        return true;
	        if (gameObjectName.Contains("POS_Money"))
		        return true;
	        if (gameObjectName.Contains("Toolbox_Door"))
		        return true;
	        if (gameObjectName.Contains("card_file_box"))
		        return true;
	        if (gameObjectName.Contains("cover_"))
		        return true;
            if (gameObjectName.Contains("lootable"))
                return true;
            if (gameObjectName.Contains("scontainer_Blue_Barrel_Base_Cap"))
		        return true;
	        if (gameObjectName.Contains("scontainer_wood_CAP"))
		        return true;
	        if (gameObjectName.Contains("suitcase_plastic_lootable_open"))
		        return true;
	        if (gameObjectName.Contains("weapon_box_cover"))
		        return true;
 
	        return false;
        }

    private static List<ulong> FindActiveObjects(string objectName, int limit = 1000000)
        {
            var actObj = imageBase;
            actObj = Memory.Read<UInt64>(actObj + 0x18);
            var fActObj = actObj;
            var ret = new List<ulong>();

            var i = 0;
            do
            {
                i++;
                if (i > limit)
                    break;

                var gObj = Memory.Read<UInt64>(actObj + 0x10);
                var gObjNamePtr = Memory.Read<UInt64>(gObj + 0x60);
                var gObjName = Memory.ReadGenericType<String>(gObjNamePtr, objectName.Length); // Read as UTF-8

                if (string.Equals(gObjName, objectName, StringComparison.CurrentCultureIgnoreCase))
                    ret.Add(gObj);

                actObj = Memory.Read<UInt64>(actObj + 0x8); // Next Object
            } while (fActObj != actObj);

            return ret;
        }

        private static JObject itemDictionary()
        {
            using (StreamReader file = File.OpenText(@"S:\Projects\Personal\ExtProject\Domain\Items.json"))
            using (JsonTextReader reader = new JsonTextReader(file))
            {
                return (JObject)JToken.ReadFrom(reader);
            }
        }
    }
}
