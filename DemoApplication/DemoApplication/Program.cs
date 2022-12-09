namespace DemoApplication
{
    internal class Program
    {
        static void Main()
        {
            int i = 0;
            while (i < 30)
            {
                F(i++);
                Thread.Sleep(1000);
            }
        }

        static void F(int i)
        {
            Console.WriteLine($"Number: {i}");
        }
    }
}
