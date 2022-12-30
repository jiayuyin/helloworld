GNU C的一大特色就是__attribute__机制。__attribute__可以设置函数属性（Function Attribute）、变量属性（Variable Attribute）和类型属性（Type Attribute）。

__attribute__书写特征是：__attribute__前后都有两个下划线，并且后面会紧跟一对括弧，括弧里面是相应的__attribute__参数。

__attribute__语法格式为：

`__attribute__ ((attribute-list))`

其位置约束：放于声明的尾部“;”之前。

packed属性：使用该属性可以使得变量或者结构体成员使用最小的对齐方式，即对变量是一字节对齐，对域（field）是位对齐。

weak:被__attribute__((weak))修饰的符号，我们称之为 弱符号（Weak Symbol）

若两个或两个以上全局符号（函数或变量名）名字一样，而其中之一声明为weak属性，则这些全局符号不会引发重定义错误。链接器会忽略弱符号，去使用普通的全局符号来解析所有对这些符号的引用，但当普通的全局符号不可用时，链接器会使用弱符号。当有函数或变量名可能被用户覆盖时，该函数或变量名可以声明为一个弱符号。