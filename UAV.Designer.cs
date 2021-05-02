using ExtProject.Domain;
using System;
using System.Windows.Forms;

namespace ExtProject
{
    partial class UAV
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private Timer timer = null;
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // timer
            // 
            this.timer.Enabled = true;
            this.timer.Interval = 250;
            this.timer.Tick += new System.EventHandler(this.timer_tick);
            // 
            // UAV
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.ClientSize = new System.Drawing.Size(1000, 1000);
            this.DoubleBuffered = true;
            this.ImeMode = System.Windows.Forms.ImeMode.On;
            this.Name = "UAV";
            this.Text = "Form1";
            this.ResumeLayout(false);

            timer.Interval = 250;
            timer.Tick += new EventHandler(timer_tick);
            timer.Start();

            this.Paint += new PaintEventHandler(EFT.FindPlayers);
        }

        public void timer_tick(object sender, EventArgs e)
        {
            this.Invalidate();
        }

        public void stop()
        {
            timer.Stop();
        }

        #endregion

    }
}